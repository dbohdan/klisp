/*
** kstring.h
** Kernel Strings
** See Copyright Notice in klisp.h
*/

#ifndef kstring_h
#define kstring_h

#include "kobject.h"
#include "kstate.h"

/* TEMP: for now all strings are mutable */
TValue kstring_new_empty(klisp_State *K);
TValue kstring_new(klisp_State *K, const char *buf, uint32_t size);
#define kstring_buf(tv_) (((Symbol *) ((tv_).tv.v.gc))->b)
#define kstring_size(tv_) (((Symbol *) ((tv_).tv.v.gc))->size)

/* The only empty string */
/* TEMP: for now initialized in ktoken.c */
TValue kempty_string;
#define kstring_is_empty(tv_) (kstring_size(tv_) == 0)

#endif
