/*
 * Alloc.c --
 *
 *	Allocator entry points used by the x11.wasm browser X11 port.
 */

#include "XlibInt.h"

void *
ckalloc(
    size_t size)
{
    void *ptr = calloc(1, size != 0 ? size : 1);

    if (ptr == NULL) {
	abort();
    }
    return ptr;
}

void
ckfree(
    void *ptr)
{
    free(ptr);
}

char *
Xpermalloc(
    unsigned int size)
{
    return (char *)ckalloc((size_t)size);
}
