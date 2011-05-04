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
    uint32_t ndigits = bigint->used;
    double accum = 0.0;
	
    /* bigint is in little endian format, but we traverse in
     big endian */
    do {
	--ndigits;
	accum = accum * radix + (double) bigint->digits[ndigits];
    } while (ndigits > 0); /* have to compare like this, it's unsigned */
    return mp_int_compare_zero(bigint) < 0? -accum : accum;
}

/* bigrat is rooted */
double kbigrat_to_double(klisp_State *K, Bigrat *bigrat)
{	
    /* TODO: check rounding in extreme cases */
    TValue tv_rem = kbigrat_copy(K, gc2bigrat(bigrat));
    krooted_tvs_push(K, tv_rem);
    Bigrat *rem = tv2bigrat(tv_rem);
    UNUSED(mp_rat_abs(K, rem, rem)); 

    TValue int_radix = kbigint_make_simple(K);
    krooted_tvs_push(K, int_radix);
    /* cant do UINT32_MAX and then +1 because _value functions take 
       int32_t arguments */ 
    UNUSED(mp_int_set_value(K, tv2bigint(int_radix), INT32_MAX)); 
    UNUSED(mp_int_add_value(K, tv2bigint(int_radix), 1, 
			    tv2bigint(int_radix)));
    UNUSED(mp_int_add(K, tv2bigint(int_radix), tv2bigint(int_radix), 
		      tv2bigint(int_radix)));

    TValue int_part = kbigint_make_simple(K);
    krooted_tvs_push(K, int_part);

    double accum = 0.0;
    double radix = (double) UINT32_MAX + 1.0;
    uint32_t digit = 0;
    /* inside there is a check to avoid problem with continuing fractions */
    while(mp_rat_compare_zero(rem) > 0) {
	UNUSED(mp_int_div(K, MP_NUMER_P(rem), MP_DENOM_P(rem), 
			  tv2bigint(int_part), NULL));

	double di = kbigint_to_double(tv2bigint(int_part));
	bool was_zero = di == 0.0;
	for (uint32_t i = 0; i < digit; i++) {
	    di /= radix;
	}
	/* if last di became 0.0 we will exit next loop,
	   this is to avoid problem with continuing fractions */
	if (!was_zero && di == 0.0)
	    break;

	++digit;
	accum += di; 
	
	UNUSED(mp_rat_sub_int(K, rem, tv2bigint(int_part), rem));
	UNUSED(mp_rat_mul_int(K, rem, tv2bigint(int_radix), rem));
    }
    krooted_tvs_pop(K); /* int_part */
    krooted_tvs_pop(K); /* int_radix */
    krooted_tvs_pop(K); /* rem */

    return mp_rat_compare_zero(bigrat) < 0? -accum : accum;
}

TValue kexact_to_inexact(klisp_State *K, TValue n)
{
    switch(ttype(n)) {
    case K_TFIXINT:
	return d2tv((double) ivalue(n));
    case K_TBIGINT: {
	Bigint *bigint = tv2bigint(n);
	double d = kbigint_to_double(bigint);
	/* d may be inf, ktag_double will handle it */
	/* MAYBE should throw an exception if strict is on */
	return ktag_double(d);
    }
    case K_TBIGRAT: {
	Bigrat *bigrat = tv2bigrat(n);
	double d = kbigrat_to_double(K, bigrat);
	return ktag_double(d);
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

/*
** read/write interface 
*/

/* TEMP: this is a stub for now, always return sufficiently large 
   number */
int32_t kdouble_print_size(TValue tv_double)
{
    UNUSED(tv_double);
    return 1024;
}

void kdouble_print_string(klisp_State *K, TValue tv_double,
			   char *buf, int32_t limit)
{
    sprintf(buf, "%f", dvalue(tv_double));
    return;
}
