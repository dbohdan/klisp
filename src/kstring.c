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
#include "kgc.h"

/* TEMP: this is for initializing the above value */
TValue kstring_new_empty(klisp_State *K)
{
    String *new_str;

    new_str = klispM_malloc(K, sizeof(String) + 1);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_str, K_TSTRING, K_FLAG_IMMUTABLE);

    /* string specific fields */
    new_str->mark = KFALSE;
    new_str->size = 0;
    new_str->b[0] = '\0';

    return gc2str(new_str);
}

/* with just size */
TValue kstring_new_s_g(klisp_State *K, bool m, uint32_t size)
{
    String *new_str;

    if (size == 0) {
	return K->empty_string;
    }

    new_str = klispM_malloc(K, sizeof(String) + size + 1);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_str, K_TSTRING, 
		m? 0 : K_FLAG_IMMUTABLE);

    /* string specific fields */
    new_str->mark = KFALSE;
    new_str->size = size;

    /* the buffer is initialized elsewhere */

    /* NOTE: all string end with a '\0' for convenience in printing 
       even if they have embedded '\0's */
    new_str->b[size] = '\0';

    return gc2str(new_str);
}

/* with buffer & size */
TValue kstring_new_bs_g(klisp_State *K, bool m, const char *buf, uint32_t size)
{
    TValue new_str = kstring_new_s_g(K, m, size);
    memcpy(kstring_buf(new_str), buf, size);
    return new_str;
}

/* with buffer but no size, no embedded '\0's */
TValue kstring_new_b_g(klisp_State *K, bool m, const char *buf)
{
    return (kstring_new_bs_g(K, m, buf, strlen(buf)));
}

/* with size and fill char */
TValue kstring_new_sf_g(klisp_State *K, bool m, uint32_t size, char fill)
{
    TValue new_str = kstring_new_s_g(K, m, size);
    memset(kstring_buf(new_str), fill, size);
    return new_str;
}

/* both obj1 and obj2 should be strings */
bool kstring_equalp(TValue obj1, TValue obj2)
{
    klisp_assert(ttisstring(obj1) && ttisstring(obj2));

    String *str1 = tv2str(obj1);
    String *str2 = tv2str(obj2);

    if (str1->size == str2->size) {
	return (str1->size == 0) ||
	    (memcmp(str1->b, str2->b, str1->size) == 0);
    } else {
	return false;
    }
}
