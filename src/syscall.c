/**
 * @file syscall.c
 * @brief system call implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-30
 */


#include <syscall.h>
#include <interrupt.h>
#include <utils.h>
#include <macros.h>


enum {
    SYSCALL_MAX_ARG_NUM = 8,
};


union syscall_args {
    size_t args[SYSCALL_MAX_ARG_NUM];
    struct mopp_args {
        char* str;
    } mopp_args;
};
typedef struct mopp_args Mopp_args;
typedef union syscall_args Syscall_args;
_Static_assert(sizeof(Syscall_args) == 32, "Syscall_args size is NOT 32 byte.");


typedef int (*Syscall_handler)(Syscall_args*);


struct syscall_entry {
    size_t args_byte_size;
    Syscall_handler func;
};
typedef struct syscall_entry Syscall_entry;


#define set_syscall_entry(num, name) \
    [num] = {\
        sizeof(struct name##_args), sys_##name,\
    }, \


static int sys_mopp(Syscall_args* a);


static Syscall_entry syscall_table[] = {
    set_syscall_entry(0, mopp)
};


void syscall_enter(Interrupt_frame* iframe) {
    Syscall_args sa;
    uint32_t code = iframe->eax;

    Syscall_entry* se = &syscall_table[code];

    /* Get process stack */
    void* pstack = (void*)iframe->prev_esp;

    /* Copy arguments */
    memcpy(&sa, pstack, se->args_byte_size);

    /* Do system call. */
    int error = se->func(&sa);

    memcpy(&iframe->eax, &error, 4);
}


static int sys_mopp(Syscall_args* a) {
    /* Mopp_args* args = (Mopp_args*)a; */

    /* printf("%s\n", args->str); */
    return 1;
}
