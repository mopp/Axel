#include <syscall_numbers.h>
#include <stddef.h>
#include <macros.h>
#include <aapi.h>
#include <point.h>


#define AXEL_API_FUNC_BODY(api_number)                   \
    {                                                    \
        int return_code = 0;                             \
        __asm__ volatile(                                \
            ".intel_syntax noprefix           \n"        \
            "mov eax, ebp                     \n"        \
            "add eax, 8                       \n"        \
            "push eax                         \n"        \
            "mov eax,  " TO_STR(api_number)  "\n"        \
            "push eax                         \n"        \
            "mov eax, " TO_STR(SYSCALL_AXEL) "\n"        \
            "int 0x80                         \n"        \
            "add esp, 8                       \n"        \
            : "=a"(return_code)                          \
            :                                            \
            : "eax");                                    \
        return return_code;                              \
    }


static int axel_api_alloc_window(Point2d const* pos, Point2d const* size)
AXEL_API_FUNC_BODY(AAPI_ALLOC_WINDOW)

static int axel_api_flush_windows(void)
AXEL_API_FUNC_BODY(AAPI_FLUSH_WINDOWS)


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
