/*
** kkeywrod.h
** Kernel Keywords
** See Copyright Notice in klisp.h
*/

#ifndef kkeyword_h
#define kkeyword_h

#include "kobject.h"
#include "kstate.h"
#include "kstring.h"
#include "kmem.h"

/* All keywords are interned */
/* No case folding is performed by these constructors */

/* buffer + size, may contain nulls */
TValue kkeyword_new_bs(klisp_State *K, const char *buf, int32_t size);
/* null terminated buffer */
TValue kkeyword_new_b(klisp_State *K, const char *buf);
/* copies str if not immutable */
TValue kkeyword_new_str(klisp_State *K, TValue str);

#define kkeyword_str(tv_) (tv2keyw(tv_)->str)
#define kkeyword_buf(tv_) (kstring_buf(tv2keyw(tv_)->str))
#define kkeyword_size(tv_) (kstring_size(tv2keyw(tv_)->str))

bool kkeywordp(TValue obj);

int32_t kkeyword_cstr_cmp(TValue str, const char *buf);

#endif
