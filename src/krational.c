/*
** krational.c
** Kernel Rationals (fixrats and bigrats)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "krational.h"
#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* This tries to convert a bigrat to a fixint or a bigint */
inline TValue kbigrat_try_integer(klisp_State *K, TValue n)
{
    Bigrat *b = tv2bigrat(n);

    if (mp_int_compare_zero(MP_DENOM_P(b)) == 0)
	return n;

    /* sadly we have to repeat the code from try_fixint here... */
    Bigint *i = MP_NUMER_P(b);
    if (MP_USED(i) == 1) {
	int64_t digit = (int64_t) *(MP_DIGITS(i));
	if (MP_SIGN(i) == MP_NEG) digit = -digit;
	if (kfit_int32_t(digit))
	    return i2tv((int32_t) digit); 
	/* else fall through */
    }
    /* should alloc a bigint */
    /* GC: n may not be rooted */
    krooted_tvs_push(K, n);
    TValue copy = kbigint_copy(K, gc2bigint(i));
    krooted_tvs_pop(K);
    return copy;
}


