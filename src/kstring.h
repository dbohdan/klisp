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

#endif
