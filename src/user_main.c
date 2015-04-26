#include <syscall_numbers.h>
#include <stddef.h>
#include <macros.h>
#include <aapi.h>
#include <point.h>


#define SYSCALL_DEFINE(func_name, syscall_num)  \
__asm__ (                                       \
    ".intel_syntax noprefix          \n"        \
    #func_name ":                    \n"        \
    "mov eax, " TO_STR(syscall_num) "\n"        \
    "int 0x80                        \n"        \
    "ret                             \n"        \
);                                              \
int func_name


SYSCALL_DEFINE(axel_api, SYSCALL_AXEL)(unsigned int n, void* args);
SYSCALL_DEFINE(sys_execve, SYSCALL_EXECVE)(char const *path, char const * const *argv, char const * const *envp);


#define AXEL_API_FUNC_BODY(api_number)    \
    {                                     \
        int return_code = 0;              \
        void* args      = NULL;           \
        __asm__ volatile(                 \
            ".intel_syntax noprefix \n"   \
            "mov eax, ebp           \n"   \
            "add eax, 8             \n"   \
            : "=a"(args)                  \
            :                             \
            : "eax");                     \
        axel_api(api_number, args);       \
        return return_code;               \
    }


static int axel_api_alloc_window(Point2d const* pos, Point2d const* size)
AXEL_API_FUNC_BODY(AAPI_ALLOC_WINDOW)

static int axel_api_flush_windows(void)
AXEL_API_FUNC_BODY(AAPI_FLUSH_WINDOWS)


int execve(char const *path, char const * const *argv, char const * const *envp) {
    return sys_execve(path, argv, envp);
}


int main(void)
{
    Point2d pos;
    Point2d size;
    pos.x  = 500;
    pos.y  = 500;
    size.x = 100;
    size.y = 100;

    int wid = axel_api_alloc_window(&pos, &size);
    axel_api_flush_windows();

    for (;;) {
        __asm__ volatile(
            ".intel_syntax noprefix \n"
            :
            : "a" (wid)
            : "eax"
                );
    }

    return 0;
}
