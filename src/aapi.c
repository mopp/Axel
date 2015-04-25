/**
 * @file aapi.c
 * @brief Axel API implement.
 * @author mopp
 * @version 0.1
 * @date 2015-04-25
 */

#include <aapi.h>
#include <window.h>
#include <rgb8.h>
#include <utils.h>


union aapi_args {
    struct alloc_window_args {
        Point2d const* pos;
        Point2d const* size;
    } alloc_window_args;
};
typedef struct alloc_window_args Alloc_window_args;
typedef union aapi_args Aapi_args;


static int aapi_alloc_window(Aapi_args*);

typedef int (*Aapi_handler)(Aapi_args*);

struct aapi_entry {
    size_t args_byte_size;
    Aapi_handler handler;
};
typedef struct aapi_entry Aapi_entry;


#define set_aapi_entry(num, name)                \
    [num] = {                                    \
        sizeof(struct name##_args), aapi_##name, \
    }

static Aapi_entry aapi_table[] = {
    set_aapi_entry(AAPI_ALLOC_WINDOW, alloc_window),
};


int axel_api_entry(unsigned int n, void* args)
{
    Aapi_entry* aentry = &aapi_table[n];

    /* Copy arguments */
    Aapi_args aa;
    memcpy(&aa, args, aentry->args_byte_size);

    return aentry->handler(&aa);
}


static int aapi_alloc_window(Aapi_args* a)
{
    Alloc_window_args* args = (Alloc_window_args*)a;

    RGB8 c = convert_color2RGB8(0xAA00AA);
    alloc_filled_window(args->pos, args->size, &c);
    flush_windows();

    return 0;
}
