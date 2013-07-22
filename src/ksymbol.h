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

/* TEMP: for now all symbols with no source info are interned */

/* NOTE: symbols can have source info, they should be compared with
   tv_sym_equal, NOT tv_equal */

/* No case folding is performed by these constructors */

/* buffer + size, may contain nulls */
TValue ksymbol_new_bs(klisp_State *K, const char *buf, uint32_t size,
                      TValue si);
/* null terminated buffer */
TValue ksymbol_new_b(klisp_State *K, const char *buf, TValue si);
/* copies str if not immutable */
TValue ksymbol_new_str(klisp_State *K, TValue str, TValue si);

#define ksymbol_str(tv_) (tv2sym(tv_)->str)
#define ksymbol_buf(tv_) (kstring_buf(tv2sym(tv_)->str))
#define ksymbol_size(tv_) (kstring_size(tv2sym(tv_)->str))

bool ksymbolp(TValue obj);
int32_t ksymbol_cstr_cmp(TValue sym, const char *buf);

#endif
