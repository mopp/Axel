/****************************************************************************************
 * @file stdio.c
 * @brief standard io functions in stl of C.
 * @author mopp
 * @version 0.1
 * @date 2014-03-17
 ****************************************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

void printf(const char* format, ...) {
    va_list args;

    va_start(args, format);

    for (const char* c = format; *c != '\0'; ++c) {
        if (*c != '%') {
            putchar(*c);
            continue;
        }

        ++c;
        switch (*c) {
            /* case '\n': */
            /* putchar('\n'); */
            case 'c':
                putchar((unsigned char)va_arg(args, int));
            case 's':
                puts(va_arg(args, const char*));
                break;
            case 'd':
            case 'i':
            case 'x': {  // add local scope
                /* INT_MAX = +32767 なので最大の5桁以上のバッファを確保 */
                char buf[10];
                puts(itoa(buf, *c, va_arg(args, int)));
                break;
            case '%':
                putchar('%');
            }
        }
    }

    va_end(args);
}
