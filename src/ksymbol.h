/*
** ksymbol.h
** Kernel Symbols
** See Copyright Notice in klisp.h
*/

#ifndef ksymbol_h
#define ksymbol_h

#include "kobject.h"
#include "kstate.h"
#include "kstring.h"
#include "kmem.h"

/* TEMP: for now all symbols are interned */
/* For identifiers */
TValue ksymbol_new_i(klisp_State *K, const char *buf, int32_t size);
/* For identifiers, simplified for unknown size */
TValue ksymbol_new(klisp_State *K, const char *buf);
/* For general strings */
TValue ksymbol_new_check_i(klisp_State *K, TValue str);

#define ksymbol_buf(tv_) (kstring_buf(tv2sym(tv_)->str))
#define ksymbol_size(tv_) (kstring_size(tv2sym(tv_)->str))

bool ksymbolp(TValue obj);

#endif
