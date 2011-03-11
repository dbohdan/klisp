/*
** kstring.c
** Kernel Strings
** See Copyright Notice in klisp.h
*/

#include <string.h>
#include <stdbool.h>

#include "kstring.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"

/* TEMP: this is for initializing the above value, for now, from ktoken.h */
TValue kstring_new_empty(klisp_State *K)
{
    String *new_str;

    new_str = klispM_malloc(K, sizeof(String) + 1);

    /* header + gc_fields */
    new_str->next = K->root_gc;
    K->root_gc = (GCObject *)new_str;
    new_str->gct = 0;
    new_str->tt = K_TSTRING;
    new_str->flags = 0;

    /* string specific fields */
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

    /* header + gc_fields */
    new_str->next = K->root_gc;
    K->root_gc = (GCObject *)new_str;
    new_str->gct = 0;
    new_str->tt = K_TSTRING;
    new_str->flags = 0;

    /* string specific fields */
    new_str->mark = KFALSE;
    new_str->size = size;
    /* NOTE: there can be embedded '\0's in a string */
    memcpy(new_str->b, buf, size);
    /* NOTE: they still end with a '\0' for convenience in printing */
    new_str->b[size] = '\0';

    return gc2str(new_str);
}

/* both obj1 and obj2 should be strings! */
bool kstring_equalp(TValue obj1, TValue obj2)
{
    String *str1 = tv2str(obj1);
    String *str2 = tv2str(obj2);

    if (str1->size == str2->size) {
	return (memcmp(str1->b, str2->b, str1->size) == 0);
    } else {
	return false;
    }
}
