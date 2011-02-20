/*
** kstring.c
** Kernel Strings
** See Copyright Notice in klisp.h
*/

/* XXX: for malloc */
#include <stdlib.h>
/* TODO: use a generalized alloc function */

#include <string.h>

#include "kstring.h"
#include "kobject.h"

/* TODO: Out of memory errors */
/* TEMP: for now all strings are mutable */
TValue kstring_new(const char *buf, uint32_t size)
{
    String *new_str = malloc(sizeof(String) + size + 1);

    new_str->next = NULL;
    new_str->gct = 0;
    new_str->tt = K_TSTRING;
    new_str->size = size;
    /* NOTE: there can be embedded '\0's in a string */
    memcpy(new_str->b, buf, size);
    /* NOTE: they still end with a '\0' for convenience in printing */
    new_str->b[size] = '\0';

    return gc2str(new_str);
}
