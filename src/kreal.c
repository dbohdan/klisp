/*
** kreal.c
** Kernel Reals (doubles)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h> 
#include <math.h>

#include "kreal.h"
#include "krational.h"
#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* MAYBE move to kobject.h */
#define ktag_double(d_)							\
    ({ double d__ = d_;							\
	TValue res;							\
	if (isnan(d__)) res = KRWNPV;					\
	else if (isinf(d__)) res = (d__ == INFINITY)? KIPINF : KIMINF;	\
	else res = d2tv(d__);						\
	res;})



double kbigint_to_double(Bigint *bigint)
{
    double radix = (double) UINT32_MAX + 1.0;
    uint32_t ndigits = bigint->used - 1;
    double accum = 0;
	
    /* bigint is in little endian format, but we traverse in
     big endian */
    while(ndigits >= 0) {
	accum = accum * radix + (double) bigint->digits[ndigits];
	--ndigits;
    }
}

TValue kexact_to_inexact(klisp_State *K, TValue n)
{
    switch(ttype(n)) {
    case K_TFIXINT:
	return d2tv((double) ivalue(n));
    case K_TBIGINT: {
	double d = kbigint_to_double(tv2bigint(n));
	/* d may be inf, ktag_double will handle it */
	/* MAYBE should throw an exception if strict is on */
	return ktag_double(d);
    }
    case K_TBIGRAT: {
	klisp_assert(0);
	return KUNDEF;
    }
    case K_TEINF:
	return tv_equal(n, KEPINF)? KIPINF : KIMINF;
    /* all of these are already inexact */
    case K_TDOUBLE:
    case K_TIINF:
    case K_TRWNPV:
    case K_TUNDEFINED:
	return n;
    default:
	klisp_assert(0);
	return KUNDEF;
    }
}
