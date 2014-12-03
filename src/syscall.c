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
#include <proc.h>


enum {
    SYSCALL_MAX_ARG_NUM = 8,
};


/* Arguments order is Left to Right. */
union syscall_args {
    size_t args[SYSCALL_MAX_ARG_NUM];
    struct mopp_args {
        char* str;
    } mopp_args;
    struct execve_args {
        char const *path;
        char const * const *argv;
        char const * const *envp;
    } execve_args;
    struct fork_args {
        uint8_t dummy;
    } fork_args;
};
typedef struct mopp_args Mopp_args;
typedef struct execve_args Execve_args;
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
    } \


static int sys_mopp(Syscall_args*);
static int sys_execve(Syscall_args*);
static int sys_fork(Syscall_args*);


static Syscall_entry syscall_table[] = {
    set_syscall_entry(0x00, mopp),
    set_syscall_entry(0x02, fork),
    set_syscall_entry(0x0b, execve),
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

    memcpy(&iframe->eax, &error, sizeof(int));
}


static int sys_mopp(Syscall_args* a) {
    (void)a;
    return 1;
}


static int sys_execve(Syscall_args* a) {
    Execve_args* args = (Execve_args*)a;

    Axel_state_code r = execve(args->path, args->argv, args->envp);

    return (r == AXEL_SUCCESS) ? (1) : (-1);
}


static int sys_fork(Syscall_args* a) {
    (void)a;
    return fork();
}
