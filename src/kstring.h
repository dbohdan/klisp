/*
** kstring.h
** Kernel Strings
** See Copyright Notice in klisp.h
*/

#ifndef kstring_h
#define kstring_h

#include "kobject.h"

/* TEMP: for now all strings are mutable */
TValue kstring_new(const char *, uint32_t);
#define kstring_buf(tv_) (((Symbol *) ((tv_).tv.v.gc))->b)
#define kstring_size(tv_) (((Symbol *) ((tv_).tv.v.gc))->size)

/* The only empty string */
/* TEMP: for now initialized in ktoken.c */
TValue kempty_string;
#define kstring_is_empty(tv_) (tv_equal(tv_, kempty_string))

#endif
