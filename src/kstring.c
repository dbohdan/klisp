/*
** kstring.c
** Kernel Strings
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "kstring.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"

/* TEMP: this is for initializing the above value, for now, from ktoken.h */
TValue kstring_new_empty(klisp_State *K)
{
    String *new_str;

    new_str = klispM_malloc(K, sizeof(String) + 1);

    new_str->next = NULL;
    new_str->gct = 0;
    new_str->tt = K_TSTRING;
    new_str->mark = KFALSE;
    new_str->size = 0;
    new_str->b[0] = '\0';

    return gc2str(new_str);
}

/* TEMP: for now all strings are mutable */
TValue kstring_new(klisp_State *K, const char *buf, uint32_t size)
{
    String *new_str;

    if (size == 0) {
	return K->empty_string;
    }

    new_str = klispM_malloc(K, sizeof(String) + size + 1);

    new_str->next = NULL;
    new_str->gct = 0;
    new_str->tt = K_TSTRING;
    new_str->mark = KFALSE;
    new_str->size = size;
    /* NOTE: there can be embedded '\0's in a string */
    memcpy(new_str->b, buf, size);
    /* NOTE: they still end with a '\0' for convenience in printing */
    new_str->b[size] = '\0';

    return gc2str(new_str);
}
