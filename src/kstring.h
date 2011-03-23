/*
** kstring.h
** Kernel Strings
** See Copyright Notice in klisp.h
*/

#ifndef kstring_h
#define kstring_h

#include <stdbool.h>

#include "kobject.h"
#include "kstate.h"

/* TEMP: for now all strings are mutable */

TValue kstring_new_empty(klisp_State *K);
TValue kstring_new(klisp_State *K, const char *buf, uint32_t size);
/* with no size, no embedded '\0's */
TValue kstring_new_ns(klisp_State *K, const char *buf);
TValue kstring_new_g(klisp_State *K, uint32_t size);
TValue kstring_new_sc(klisp_State *K, uint32_t size, char fill);

#define kstring_buf(tv_) (tv2str(tv_)->b)
#define kstring_size(tv_) (tv2str(tv_)->size)

#define kstring_is_empty(tv_) (kstring_size(tv_) == 0)

/* both obj1 and obj2 should be strings! */
bool kstring_equalp(TValue obj1, TValue obj2);

#endif
