/**
 * @file elf.c
 * @brief elf format
 * @author mopp
 * @version 0.1
 * @date 2014-11-23
 */
/**
 * @file elf.h
 * @brief elf
 * @author mopp
 * @version 0.1
 * @date 2014-11-23
 */

#ifndef _ELF_H_
#define _ELF_H_

#if __x86_64__
#define _ENV_64BIT
#else
#define _ENV_32BIT
#endif



#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <state_code.h>

/*
 * 32 bit format.
 * +---------------+------+-----------+--------------------------+
 * |Name           | Size | Alignment | Purpose                  |
 * +---------------+------+-----------+--------------------------+
 * | Elf32_Addr    |    4 |         4 | Unsigned program address |
 * | Elf32_Off     |    4 |         4 | Unsigned file offset     |
 * | Elf32_Half    |    2 |         2 | Unsigned medium integer  |
 * | Elf32_Word    |    4 |         4 | Unsigned large integer   |
 * | Elf32_Sword   |    4 |         4 | Signed large integer     |
 * | unsigned char |    1 |         1 | Unsigned small integer   |
 * +------------- -+------+-----------+--------------------------+
 */

/*
 * 64 bit format.
 * +---------------+------+-----------+--------------------------+
 * |Name           | Size | Alignment | Purpose                  |
 * +---------------+------+-----------+--------------------------+
 * | Elf64_Addr    |    8 |         8 | Unsigned program address |
 * | Elf64_Off     |    8 |         8 | Unsigned file offset     |
 * | Elf64_Half    |    2 |         2 | Unsigned medium integer  |
 * | Elf64_Word    |    4 |         4 | Unsigned integer         |
 * | Elf64_Sword   |    4 |         4 | Signed integer           |
 * | Elf64_Xword   |    8 |         8 | Unsigned long integer    |
 * | Elf64_Sxword  |    8 |         8 | Signed long integer      |
 * | unsigned char |    1 |         1 | Unsigned small integer   |
 * +---------------+------+-----------+--------------------------+
 */


typedef uint32_t elf32_addr;
typedef uint32_t elf32_off;
typedef uint16_t elf32_half;
typedef uint32_t elf32_word;
typedef int32_t  elf32_sword;
typedef uint8_t  elf32_uchar;

typedef uint64_t elf64_addr;
typedef uint64_t elf64_off;
typedef uint16_t elf64_half;
typedef uint32_t elf64_word;
typedef int32_t  elf64_sword;
typedef uint64_t elf64_xword;
typedef int64_t  elf64_sxword;
typedef uint8_t  elf64_uchar;


#ifdef _ENV_64BIT
typedef elf64_addr   elf_addr;
typedef elf64_off    elf_off;
typedef elf64_half   elf_half;
typedef elf64_word   elf_word;
typedef elf64_sword  elf_sword;
typedef elf64_uchar  elf_uchar;
#else
typedef elf32_addr   elf_addr;
typedef elf32_off    elf_off;
typedef elf32_half   elf_half;
typedef elf32_word   elf_word;
typedef elf32_sword  elf_sword;
typedef elf32_uchar  elf_uchar;
#endif
typedef elf64_xword  elf_xword;
typedef elf64_sxword elf_sxword;


struct elf_phdr {
    elf_word type;
    elf_off offset;
    elf_addr vaddr;
    elf_addr paddr;
    elf_word filesz;
    elf_word memsz;
    elf_word flags;
    elf_word align;
};
typedef struct elf_phdr Elf_phdr;


enum {
    PT_NULL        = 0,
    PT_LOAD        = 1,
    PT_DYNAMIC     = 2,
    PT_INTERP      = 3,
    PT_NOTE        = 4,
    PT_SHLIB       = 5,
    PT_PHDR        = 6,
    PT_LOPROC      = 0x70000000,
    PT_HIPROC      = 0x7FFFFFFF,
};

typedef Axel_state_code (*Elf_load_callback)(void* p, size_t, Elf_phdr const*);
typedef Axel_state_code (*Elf_load_err_callback)(void* p, size_t, Elf_phdr const*);


Axel_state_code elf_load_program(void const*, void*, Elf_load_callback, Elf_load_err_callback);



#endif
