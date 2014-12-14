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


uint8_t* fat_data_cluster_access(Fat_manips const* fm, uint8_t direction, uint32_t clus_num, uint8_t* buffer) {
    uint8_t spc = fm->bpb->sec_per_clus;
    uint32_t sec = fm->data.begin_sec + ((clus_num - 2) * spc);

    if (fm->b_access(fm->obj, direction, sec, spc, buffer) != AXEL_SUCCESS) {
        return NULL;
    }

    return buffer;
}


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
