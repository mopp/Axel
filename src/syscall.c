/**
 * @file syscall.c
 * @brief system call implementation.
 * @author mopp
 * @version 0.1
 * @date 2014-10-30
 */


#include <syscall.h>
#include <syscall_numbers.h>
#include <interrupt.h>
#include <utils.h>
#include <macros.h>
#include <proc.h>
#include <aapi.h>


enum {
    SYSCALL_MAX_ARG_NUM = 8,
};


/* Arguments order is Left to Right. */
union syscall_args {
    size_t args[SYSCALL_MAX_ARG_NUM];

    struct axel_args {
        unsigned int api_number;
        void* api_arguments;
    } axel_args;

    struct execve_args {
        char const* path;
        char const* const* argv;
        char const* const* envp;
    } execve_args;

    struct fork_args {
        uint8_t dummy;
    } fork_args;
};
typedef struct axel_args Axel_args;
typedef struct execve_args Execve_args;
typedef union syscall_args Syscall_args;
_Static_assert(sizeof(Syscall_args) == 32, "Syscall_args size is NOT 32 byte.");


/* System call handler prototypes for Syscall_entry. */
static int sys_axel(Syscall_args*);
static int sys_execve(Syscall_args*);
static int sys_fork(Syscall_args*);

typedef int (*Syscall_handler)(Syscall_args*);

struct syscall_entry {
    size_t args_byte_size;   /* Byte size of arguments */
    Syscall_handler handler; /* Pointer to system call handler function */
};
typedef struct syscall_entry Syscall_entry;


#define set_syscall_entry(num, name)            \
    [num] = {                                   \
        sizeof(struct name##_args), sys_##name, \
    }

static Syscall_entry syscall_table[] = {
    set_syscall_entry(SYSCALL_AXEL, axel),
    set_syscall_entry(SYSCALL_FORK, fork),
    set_syscall_entry(SYSCALL_EXECVE, execve),
};


/**
 * @brief entry function of all system call
 * @param iframe this is used for obtaining system call number and process stack.
 */
void syscall_enter(Interrupt_frame* iframe)
{
    /* Get system call entry. */
    uint32_t const syscall_number = iframe->eax;
    Syscall_entry const* const sentry = &syscall_table[syscall_number];

    /* Get process stack for arguments */
    void* process_stack = (void*)iframe->prev_esp;

    /* Copy arguments */
    Syscall_args sa;
    memcpy(&sa, process_stack, sentry->args_byte_size);

    /* Do system call. */
    int error = sentry->handler(&sa);

    /* Set return value. */
    memcpy(&iframe->eax, &error, sizeof(int));
}


static int sys_axel(Syscall_args* a)
{
    Axel_args* args = (Axel_args*)a;
    return axel_api_entry(args->api_number, args->api_arguments);
}


static int sys_execve(Syscall_args* a)
{
    Execve_args* args = (Execve_args*)a;

    Axel_state_code r = execve(args->path, args->argv, args->envp);

    return (r == AXEL_SUCCESS) ? (1) : (-1);
}


static int sys_fork(Syscall_args* a)
{
    (void)a;
    return fork();
}
