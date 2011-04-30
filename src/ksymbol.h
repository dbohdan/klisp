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

/* For identifiers */
TValue ksymbol_new_i(klisp_State *K, const char *buf, int32_t size,
    TValue si);
/* For identifiers, simplified for unknown size */
TValue ksymbol_new(klisp_State *K, const char *buf, TValue si);
/* For general strings, copies str if not immutable */
TValue ksymbol_new_check_i(klisp_State *K, TValue str, TValue si);

#define ksymbol_str(tv_) (tv2sym(tv_)->str)
#define ksymbol_buf(tv_) (kstring_buf(tv2sym(tv_)->str))
#define ksymbol_size(tv_) (kstring_size(tv2sym(tv_)->str))

bool ksymbolp(TValue obj);

#endif
