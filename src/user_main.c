#include <syscall_numbers.h>
#include <stddef.h>
#include <macros.h>
#include <aapi.h>
#include <point.h>


static int axel_api_alloc_window(Point2d const*, Point2d const*);
static int axel_api_flush_windows(void);


int main(void)
{
    Point2d pos;
    Point2d size;
    pos.x  = 500;
    pos.y  = 500;
    size.x = 100;
    size.y = 100;
    axel_api_alloc_window(&pos, &size);
    axel_api_flush_windows();

    for (;;) {
        __asm__ volatile("movl $16, %ecx");
    }

    return 0;
}


#define AXEL_API_TEMPLATE(return_code_var, api_number) \
    __asm__ volatile(                                  \
        ".intel_syntax noprefix           \n"          \
        "mov eax, ebp                     \n"          \
        "add eax, 8                       \n"          \
        "push eax                         \n"          \
        "mov eax,  " TO_STR(api_number)  "\n"          \
        "push eax                         \n"          \
        "mov eax, " TO_STR(SYSCALL_AXEL) "\n"          \
        "int 0x80                         \n"          \
        "add esp, 8                       \n"          \
        : "=a"(return_code_var)                        \
        :                                              \
        : "eax")


static int axel_api_alloc_window(Point2d const* pos, Point2d const* size)
{
    int return_code = 0;

    AXEL_API_TEMPLATE(return_code, AAPI_ALLOC_WINDOW);

    return return_code;
}


static int axel_api_flush_windows(void)
{
    int return_code = 0;

    AXEL_API_TEMPLATE(return_code, AAPI_FLUSH_WINDOWS);

    return return_code;
}
