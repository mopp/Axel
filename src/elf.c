#include <elf.h>


enum {
    EI_MAG0        = 0,
    EI_MAG1        = 1,
    EI_MAG2        = 2,
    EI_MAG3        = 3,
    EI_CLASS       = 4,
    EI_DATA        = 5,
    EI_VERSION     = 6,
    EI_PAD         = 7,
    EI_NIDENT      = 16,

    ELF_MAG0       = 0x7F,
    ELF_MAG1       = 'E',
    ELF_MAG2       = 'L',
    ELF_MAG3       = 'F',

    ELF_CLASSNONE  = 0,
    ELF_CLASS32    = 1,
    ELF_CLASS64    = 2,

    ELF_DATANONE   = 0,
    ELF_DATA2LSB   = 1,
    ELF_DATA2MSB   = 2,

    ET_NONE        = 0,        /* Unknown file type. */
    ET_REL         = 1,         /* Relocatable file type. */
    ET_EXEC        = 2,        /* Executable file type. */
    ET_DYN         = 3,         /* Share object file type. */
    ET_CORE        = 4,        /* Core file type. */
    ET_LOPROC      = 0xFF00, /* Depend on processor. */
    ET_HIPROC      = 0xFFFF, /* Depend on processor. */

    EM_NONE        = 0,         /* Unknown */
    EM_M32         = 1,          /* AT&T WE 32100 */
    EM_SPARC       = 2,        /* SPARC */
    EM_386         = 3,          /* Intel */
    EM_68K         = 4,          /* motorola 68000 */
    EM_88K         = 5,          /* motorola 88000 */
    ET_860         = 7,          /* Intel 80860 */
    EM_MIPS        = 8,         /* MIPS RS 3000 */
    EM_MIPS_RS4_BE = 10, /* MIPS RS 4000 */

    EV_NONE        = 0,
    EV_CURRENT     = 1,

    SHN_UNDEF      = 0,
    SHN_LORESERVE  = 0xFF00,
    SHN_LOPROC     = 0xFF00,
    SHN_HIPROC     = 0xFF1F,
    SHN_ABS        = 0xFFF1,
    SHN_COMMON     = 0xFFF2,
    SHN_HIRESERVE  = 0xFFFF,

    /* For section header */
    SHT_NULL       = 0,
    SHT_PROGBITS   = 1,
    SHT_SYMTAB     = 2,
    SHT_STRTAB     = 3,
    SHT_RELA       = 4,
    SHT_HASH       = 5,
    SHT_DYNAMIC    = 6,
    SHT_NOTE       = 7,
    SHT_NOBITS     = 8,
    SHT_REL        = 9,
    SHT_SHLIB      = 10,
    SHT_DYNSYM     = 11,
    SHT_LOPROC     = 0x70000000,
    SHT_HIPROC     = 0x7FFFFFFF,
    SHT_LOUSER     = 0x80000000,
    SHT_HIUSER     = 0x8FFFFFFF,
};


struct elf_ehdr {
    elf_uchar ident[EI_NIDENT];
    elf_half type;
    elf_half machine;
    elf_word version;
    elf_addr entry;
    elf_off ph_off;
    elf_off sh_off;
    elf_word flags;
    elf_half eh_size;
    elf_half ph_entry_size;
    elf_half ph_num;
    elf_half sh_entry_size;
    elf_half sh_num;
    elf_half sh_str_idx;
};
typedef struct elf_ehdr Elf_ehdr;


struct elf_shdr {
    elf_word name;
    elf_word type;
    elf_word flags;
    elf_addr addr;
    elf_off offset;
    elf_word size;
    elf_word link;
    elf_word info;
    elf_word addr_align;
    elf_word ent_size;
};
typedef struct elf_shdr Elf_shdr;


struct elf_sym {
    elf_word name;
    elf_addr value;
    elf_word size;
    elf_uchar info;
    elf_uchar other;
    elf_half shndx;
};
typedef struct elf_sym Elf_sym;


struct elf_rel {
    elf_addr offset;
    elf_word info;
};
typedef struct elf_rel Elf_rel;


struct elf_rela {
    elf_addr offset;
    elf_word info;
    elf_sword addend;
};
typedef struct elf_rela Elf_rela;


static bool is_valid_elf(elf_uchar const* id) {
    return ((id[EI_MAG0] == ELF_MAG0) && (id[EI_MAG1] == ELF_MAG1) && (id[EI_MAG2] == ELF_MAG2) && (id[EI_MAG3] == ELF_MAG3)) ? (true) : (false);
}


Axel_state_code elf_load_program(void const* fbuf, void* o, Elf_load_callback f, Elf_load_err_callback ef) {
    Elf_ehdr const* eh = (Elf_ehdr const*)(fbuf);

    /* TODO: divide error type. */
    if ((fbuf == NULL) || (f == NULL)) {
        return AXEL_FAILED;
    }

    if (is_valid_elf(eh->ident) == false) {
        return AXEL_FAILED;
    }

    if (eh->type != ET_EXEC) {
        return AXEL_FAILED;
    }

    if (eh->machine != EM_386) {
        return AXEL_FAILED;
    }

    if ((eh->ph_num == 0) || (eh->sh_num == 0) || (eh->ph_entry_size == 0) || (eh->sh_entry_size == 0)) {
        return AXEL_FAILED;
    }

    /* TODO: more flexible. */
    Elf_phdr const* ph = (Elf_phdr const*)((uintptr_t)fbuf + eh->ph_off);
    uint8_t i = 0;
    while (i < 2) {
        if (f(o, i, fbuf, ph) != AXEL_SUCCESS) {
            ef(o, i, fbuf, ph);
            return AXEL_FAILED;
        }
        i++;

        /* next */
        ph = (Elf_phdr const*)((uintptr_t)ph + eh->ph_entry_size);
    }

    return AXEL_SUCCESS;
}

uintptr_t elf_get_entry_addr(void const* fbuf) {
    Elf_ehdr const* eh = (Elf_ehdr const*)(fbuf);

    /* TODO: divide error type. */
    if (fbuf == NULL) {
        return AXEL_FAILED;
    }

    if (is_valid_elf(eh->ident) == false) {
        return AXEL_FAILED;
    }

    if (eh->type != ET_EXEC) {
        return AXEL_FAILED;
    }

    if (eh->machine != EM_386) {
        return AXEL_FAILED;
    }

    if ((eh->ph_num == 0) || (eh->sh_num == 0) || (eh->ph_entry_size == 0) || (eh->sh_entry_size == 0)) {
        return AXEL_FAILED;
    }

    return eh->entry;
}
