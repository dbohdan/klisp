/*
** ksymbol.h
** Kernel Symbols
** See Copyright Notice in klisp.h
*/

#ifndef ksymbol_h
#define ksymbol_h

#include "kobject.h"
#include "kstate.h"
#include "kmem.h"

/* TEMP: for now all symbols are interned */
TValue ksymbol_new(klisp_State *K, const char *buf);

#define ksymbol_buf(tv_) (((Symbol *) ((tv_).tv.v.gc))->b)

#endif
