#include "./include/mbr.h"
#include "./include/fat_manip.h"
#include "./include/utils.h"


static inline void* alloc_clus_buf(Fat_manips const* fm) {
    return fm->alloc(fm->byte_per_cluster);
}


/**
 * @brief Access FAT entry.
 * @param fm manipulator
 * @param direction read or write
 * @param n FAT Number to read/write
 * @param write_entry It is enable when writting.
 * @return Value of FAT entry.
 */
uint32_t fat_enrty_access(Fat_manips const* fm, uint8_t direction, uint32_t n, uint32_t write_entry) {
    uint32_t offset = n * ((fm->fat_type == FAT_TYPE32) ? 4 : 2);
    uint32_t bps = fm->bpb->bytes_per_sec;
    uint32_t sec_num = fm->bpb->rsvd_area_sec_num + (offset / bps);
    uint32_t entry_offset = offset % bps;
    void* buf = alloc_clus_buf(fm);
    if (buf == NULL) {
        return 0;
    }

    if (fm->b_access(fm->obj, FILE_READ, sec_num, 1, buf) != AXEL_SUCCESS) {
        goto failed;
    }
    uint32_t* b = (uint32_t*)((uintptr_t)buf + entry_offset);
    uint32_t read_entry = 0;

    /* Filter */
    switch (fm->fat_type) {
        case FAT_TYPE12:
            read_entry = (*b & 0xFFF);
            break;
        case FAT_TYPE16:
            read_entry = (*b & 0xFFFF);
            break;
        case FAT_TYPE32:
            read_entry = (*b & 0x0FFFFFFF);
            break;
        default:
            /* Error */
            goto failed;
    }

    if (direction == FILE_READ) {
        fm->free(buf);
        return read_entry;
    }

    /* Change FAT entry. */
    switch (fm->fat_type) {
        case FAT_TYPE12:
            *b = (write_entry & 0xFFF);
            break;
        case FAT_TYPE16:
            *b = (write_entry & 0xFFFF);
            break;
        case FAT_TYPE32:
            /* keep upper bits */
            *b = (*b & 0xF0000000) | (write_entry & 0x0FFFFFFF);
            break;
        default:
            /* Error */
            goto failed;
    }

    if (fm->b_access(fm->obj, FILE_WRITE, sec_num, 1, buf) != AXEL_SUCCESS) {
        goto failed;
    }

    return *b;

failed:
    fm->free(buf);
    return 0;
}


uint32_t set_last_fat_entry(Fat_manips const* fm, uint32_t n) {
    uint32_t last = 0;

    switch (fm->fat_type) {
        case FAT_TYPE12:
            last = 0xFFF;
        case FAT_TYPE16:
            last = 0xFFFF;
        case FAT_TYPE32:
            last = 0x0FFFFFFF;
    }

    return fat_enrty_access(fm, FILE_WRITE, n, last);
}


Fsinfo* fat_fsinfo_access(Fat_manips const* fm, uint8_t direction, Fsinfo* fsi) {
    void* buf = alloc_clus_buf(fm);
    if (buf == NULL) {
        return NULL;
    }

    if (direction == FILE_WRITE) {
        memcpy(buf, fsi, sizeof(Fsinfo));
    }

    /* Access FSINFO structure. */
    if (fm->b_access(fm->obj, direction, fm->bpb->fat32.fsinfo_sec_num, 1, buf) != AXEL_SUCCESS) {
        fm->free(buf);
        return NULL;
    }

    if (direction == FILE_READ) {
        memcpy(fsi, buf, sizeof(Fsinfo));
    }

    fm->free(buf);

    return fsi;
}


uint8_t* fat_data_cluster_access(Fat_manips const* fm, uint8_t direction, uint32_t cluster_number, uint8_t* buffer) {
    uint8_t spc = fm->bpb->sec_per_clus;
    uint32_t sec = fm->area.data.begin_sec + ((cluster_number - 2) * spc);

    if (fm->b_access(fm->obj, direction, sec, spc, buffer) != AXEL_SUCCESS) {
        return NULL;
    }

    return buffer;
}


Fat_area* fat_calc_sectors(Bios_param_block const* bpb, Fat_area* fa) {
    /* These are logical sector number. */
    uint32_t fat_size = (bpb->fat_size16 != 0) ? (bpb->fat_size16) : (bpb->fat32.fat_size32);
    uint32_t total_sec = (bpb->total_sec16 != 0) ? (bpb->total_sec16) : (bpb->total_sec32);

    fa->rsvd.begin_sec = 0;
    fa->rsvd.sec_nr = bpb->rsvd_area_sec_num;

    fa->fat.begin_sec = fa->rsvd.begin_sec + fa->rsvd.sec_nr;
    fa->fat.sec_nr = fat_size * bpb->num_fats;

    fa->rdentry.begin_sec = fa->fat.begin_sec + fa->fat.sec_nr;
    fa->rdentry.sec_nr = (32 * bpb->root_ent_cnt + bpb->bytes_per_sec - 1) / bpb->bytes_per_sec;

    fa->data.begin_sec = fa->rdentry.begin_sec + fa->rdentry.sec_nr;
    fa->data.sec_nr = total_sec - fa->data.begin_sec;

    return fa;
}


uint32_t alloc_cluster(Fat_manips* fm) {
    if (fm->fsinfo->free_cnt == 0) {
        /* No free cluster. */
        return 0;
    }

    uint32_t begin = fm->fsinfo->next_free + 1;
    uint32_t end = fm->area.data.sec_nr / fm->bpb->sec_per_clus;
    uint32_t i, select_clus = 0;

    for (i = begin; i < end; i++) {
        uint32_t fe = fat_enrty_access(fm, FILE_READ, i, 0);
        if (is_unused_fat_entry(fe) == true) {
            /* Found unused cluster. */
            select_clus = i;
            break;
        }
    }

    if (select_clus == 0) {
        /* Unused cluster is not found. */
        return 0;
    }

    /* Update FSINFO. */
    fm->fsinfo->next_free = select_clus;
    fm->fsinfo->free_cnt -= 1;
    fat_fsinfo_access(fm, FILE_WRITE, fm->fsinfo);

    return select_clus;
}


void free_cluster(Fat_manips* fm, uint32_t clus) {
    uint32_t fe;
    do {
        fe = fat_enrty_access(fm, FILE_READ, clus, 0);
        bool f = is_unused_fat_entry(fe);
        if (f == false || is_last_fat_entry(fm, fe) == true) {
            fat_enrty_access(fm, FILE_WRITE, clus, 0);
        }

        if (f == true) {
            return;
        }
        clus = fe;
    } while (1);
}


/* Axel_state_code access_fat_file(Fat_manips* fm, uint8_t direction, ) { */
/* } */


bool is_valid_fsinfo(Fsinfo* fsi) {
    return ((fsi->lead_signature != FSINFO_LEAD_SIG) || (fsi->struct_signature != FSINFO_STRUCT_SIG) || (fsi->trail_signature != FSINFO_TRAIL_SIG)) ? false : true;
}


bool is_unused_fat_entry(uint32_t fe) {
    return (fe == 0) ? true : false;
}


bool is_rsvd_fat_entry(uint32_t fe) {
    return (fe == 1) ? true : false;
}


bool is_bad_clus_fat_entry(Fat_manips const* fm, uint32_t fe) {
    switch (fm->fat_type) {
        case FAT_TYPE12:
            return (fe == 0xFF7) ? true : false;
        case FAT_TYPE16:
            return (fe == 0xFFF7) ? true : false;
        case FAT_TYPE32:
            return (fe == 0x0FFFFFF7) ? true : false;
    }

    return false;
}


bool is_valid_data_fat_entry(Fat_manips const* fm, uint32_t fe) {
    /* NOT bad cluster and reserved. */
    return (is_bad_clus_fat_entry(fm, fe) == false && is_rsvd_fat_entry(fe) == false) ? true : false;
}


bool is_data_exist_fat_entry(Fat_manips const* fm, uint32_t fe) {
    return (is_unused_fat_entry(fe) == false && is_valid_data_fat_entry(fm, fe) == true) ? true : false;
}


bool is_last_fat_entry(Fat_manips const* fm, uint32_t fe) {
    switch (fm->fat_type) {
        case FAT_TYPE12:
            return (0xFF8 <= fe && fe <= 0xFFF) ? true : false;
        case FAT_TYPE16:
            return (0xFFF8 <= fe && fe <= 0xFFFF) ? true : false;
        case FAT_TYPE32:
            return (0x0FFFFFF8 <= fe && fe <= 0x0FFFFFFF) ? true : false;
    }

    /* ERROR. */
    return false;
}
