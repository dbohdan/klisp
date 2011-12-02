/*
** krational.c
** Kernel Rationals (fixrats and bigrats)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h> /* for code checking '/' & '.' */
#include <inttypes.h>
#include <ctype.h> /* for to lower */
#include <math.h>

#include "krational.h"
#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* used for res & temps in operations */
/* NOTE: This is to be called only with already reduced values */
TValue kbigrat_new(klisp_State *K, bool sign, uint32_t num, 
		   uint32_t den)
{
    Bigrat *new_bigrat = klispM_new(K, Bigrat);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_bigrat, K_TBIGRAT, 0);

    /* bigrat specific fields */
    /* If later changed to alloc obj: 
       GC: root bigint & put dummy value to work if garbage collections 
       happens while allocating array */
    new_bigrat->num.single = num;
    new_bigrat->num.digits = &(new_bigrat->num.single);
    new_bigrat->num.alloc = 1;
    new_bigrat->num.used = 1;
    new_bigrat->num.sign = sign? MP_NEG : MP_ZPOS;

    new_bigrat->den.single = den;
    new_bigrat->den.digits = &(new_bigrat->den.single);
    new_bigrat->den.alloc = 1;
    new_bigrat->den.used = 1;
    new_bigrat->den.sign = MP_ZPOS;

    return gc2bigrat(new_bigrat);
}

/* assumes src is rooted */
TValue kbigrat_copy(klisp_State *K, TValue src)
{
    TValue copy = kbigrat_make_simple(K);
    /* arguments are in reverse order with respect to mp_rat_copy */
    UNUSED(mp_rat_init_copy(K, tv2bigrat(copy), tv2bigrat(src)));
    return copy;
}

/*
** read/write interface 
*/
/* this works for bigrats, bigints & fixints, returns true if ok */
bool krational_read(klisp_State *K, char *buf, int32_t base, TValue *out, 
		   char **end)
{
    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    bool ret_val = (mp_rat_read_cstring(K, tv2bigrat(res), base, 
					buf, end) == MP_OK);
    krooted_tvs_pop(K);
    *out = kbigrat_try_integer(K, res);

    /* TODO: ideally this should be incorporated in the read code */
    /* detect sign after '/', and / before numbers, those are allowed 
       by imrat but not in kernel */
    if (ret_val) {
	char *slash = strchr(buf, '/');
	if (slash != NULL && (slash == 0 || 
			      (*(slash+1) == '+' || *(slash+1) == '-') ||
			      (*(slash-1) == '+' || *(slash-1) == '-')))
	    ret_val = false;
    }

    return ret_val;
}

/* NOTE: allow decimal for use after #e */
bool krational_read_decimal(klisp_State *K, char *buf, int32_t base, TValue *out, 
			    char **end, bool *out_decimalp)
{
    /* NOTE: in Kernel only base ten is allowed in decimal format */
    klisp_assert(base == 10);

    char *my_end;
    if (!end) /* always get the last char not read */
	end = &my_end;

    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    mp_result ret_val = mp_rat_read_ustring(K, tv2bigrat(res), base, 
					buf, end);

    /* check to see if there was a decimal point, will only
     be written to out_decimalp if no error ocurr */
    /* TEMP: mp_rat_read_ustring does not set *end if an error occurs.
     * Do not let memchr() read past the end of the buffer. */
    bool decimalp = (ret_val == MP_OK || ret_val == MP_TRUNC)
      ? (memchr(buf, '.', *end - buf) != NULL)
      : false;

    /* handle exponents, avoid the case where the number finishes
     in a decimal point (this isn't allowed by kernel */
    if (decimalp && ret_val == MP_TRUNC && *end != buf &&
	  *((*end)-1) != '.') {
	char *ebuf = *end;
	char el = tolower(*ebuf);
	/* NOTE: in klisp all exponent letters map to double */
	if (el == 'e' || el == 's' || el == 'f' || el == 'd' || el == 'l') {
	    ebuf++;
	    TValue tv_exp_exp = kbigint_make_simple(K);
	    krooted_tvs_push(K, tv_exp_exp);
	    Bigint *exp_exp = tv2bigint(tv_exp_exp);
	    /* base should be 10 */
	    ret_val = mp_int_read_cstring(K, exp_exp, base, ebuf, end);
	    if (ret_val == MP_OK) {
		/* IMath doesn't have general rational exponentiation,
		   so do it manually */
		TValue tv_iexp = kbigint_make_simple(K);
		krooted_tvs_push(K, tv_iexp);
		Bigint *iexp = tv2bigint(tv_iexp);
		UNUSED(mp_int_set_value(K, iexp, 10));
		bool negativep = mp_int_compare_zero(exp_exp) < 0;
		UNUSED(mp_int_abs(K, exp_exp, exp_exp));
		UNUSED(mp_int_expt_full(K, iexp, exp_exp, iexp));
		/* iexp has 10^|exp_exp| */
		if (negativep) {
		    TValue tv_rexp = kbigrat_make_simple(K);
		    krooted_tvs_push(K, tv_rexp);
		    Bigrat *rexp = tv2bigrat(tv_rexp);
		    /* copy reciprocal of iexp to rexp */
		    UNUSED(mp_rat_zero(K, rexp));
		    UNUSED(mp_rat_add_int(K, rexp, iexp, rexp));
		    UNUSED(mp_rat_recip(K, rexp, rexp));
		    /* now get true number */
		    UNUSED(mp_rat_mul(K, tv2bigrat(res), rexp,
				      tv2bigrat(res)));
		    krooted_tvs_pop(K); /* rexp */
		} else { /* exponent positive, can just mult int */
		    UNUSED(mp_rat_mul_int(K, tv2bigrat(res), iexp,
					  tv2bigrat(res)));
		}
		krooted_tvs_pop(K); /* iexp */
		/* fall through, ret_val remains MP_OK */
	    } /* else, fall through, ret_val remains != MP_OK */
	    krooted_tvs_pop(K); /* exp_exp */
	}
    }

    /* TODO: ideally this should be incorporated in the read code */
    /* TODO: if returning MP_TRUNC adjust the end pointer returned */
    /* detect sign after '/', or trailing '.' or starting '/' or '.'. 
       Those are allowed by imrat but not by kernel */
    if (ret_val == MP_OK) {
	char *ch = strchr(buf, '/');
	if (ch != NULL && (ch == 0 || 
			   (*(ch+1) == '+' || *(ch+1) == '-') ||
			   (*(ch-1) == '+' || *(ch-1) == '-'))) {
	    ret_val = MP_TRUNC;
	} else {
	    ch = strchr(buf, '.');
	    if (ch != NULL && (ch == 0 || 
			       (*(ch+1) == '+' || *(ch+1) == '-') ||
			       (*(ch-1) == '+' || *(ch-1) == '-') ||
			       ch == *end - 1)) {
		ret_val = MP_TRUNC;
	    }
	}
    }

    if (ret_val == MP_OK && out_decimalp != NULL)
	*out_decimalp = decimalp;

    krooted_tvs_pop(K);
    if (ret_val == MP_OK) {
	*out = kbigrat_try_integer(K, res);
    }

    return ret_val == MP_OK;
}

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigrat_print_size(TValue tv_bigrat, int32_t base)
{
    klisp_assert(ttisbigrat(tv_bigrat));
    return mp_rat_string_len(tv2bigrat(tv_bigrat), base);
}

/* this is used by write */
void  kbigrat_print_string(klisp_State *K, TValue tv_bigrat, int32_t base, 
			   char *buf, int32_t limit)
{
    klisp_assert(ttisbigrat(tv_bigrat));
    mp_result res = mp_rat_to_string(K, tv2bigrat(tv_bigrat), base, buf, 
				     limit);
    /* only possible error is truncation */
    klisp_assert(res == MP_OK);
}

/* Interface for kgnumbers */

/* The compare predicates take a klisp_State because in general
   may need to do multiplications */
bool kbigrat_eqp(klisp_State *K, TValue tv_bigrat1, TValue tv_bigrat2)
{
    return (mp_rat_compare(K, tv2bigrat(tv_bigrat1), 
			   tv2bigrat(tv_bigrat2)) == 0);
}

bool kbigrat_ltp(klisp_State *K, TValue tv_bigrat1, TValue tv_bigrat2)
{
    return (mp_rat_compare(K, tv2bigrat(tv_bigrat1), 
			   tv2bigrat(tv_bigrat2)) < 0);
}

bool kbigrat_lep(klisp_State *K, TValue tv_bigrat1, TValue tv_bigrat2)
{
    return (mp_rat_compare(K, tv2bigrat(tv_bigrat1), 
			   tv2bigrat(tv_bigrat2)) <= 0);
}

bool kbigrat_gtp(klisp_State *K, TValue tv_bigrat1, TValue tv_bigrat2)
{
    return (mp_rat_compare(K, tv2bigrat(tv_bigrat1), 
			   tv2bigrat(tv_bigrat2)) > 0);
}

bool kbigrat_gep(klisp_State *K, TValue tv_bigrat1, TValue tv_bigrat2)
{
    return (mp_rat_compare(K, tv2bigrat(tv_bigrat1), 
			   tv2bigrat(tv_bigrat2)) >= 0);
}

/*
** GC: All of these assume the parameters are rooted 
*/
TValue kbigrat_plus(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_rat_add(K, tv2bigrat(n1), tv2bigrat(n2), tv2bigrat(res)));
    krooted_tvs_pop(K);
    return kbigrat_try_integer(K, res);
}

TValue kbigrat_times(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_rat_mul(K, tv2bigrat(n1), tv2bigrat(n2), tv2bigrat(res)));
    krooted_tvs_pop(K);
    return kbigrat_try_integer(K, res);
}

TValue kbigrat_minus(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_rat_sub(K, tv2bigrat(n1), tv2bigrat(n2), tv2bigrat(res)));
    krooted_tvs_pop(K);
    return kbigrat_try_integer(K, res);
}

/* NOTE: n2 can't be zero, that case should be checked before calling this */
TValue kbigrat_div_mod(klisp_State *K, TValue n1, TValue n2, TValue *res_r)
{
    /* NOTE: quotient is always an integer, remainder may be any rational */
    TValue tv_q = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_q);
    TValue tv_r = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_r);
    /* for temp values */
    TValue tv_true_rem = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_true_rem);
    TValue tv_div = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_div);

    Bigrat *n = tv2bigrat(n1);
    Bigrat *d = tv2bigrat(n2);

    Bigint *q = tv2bigint(tv_q);
    Bigint *r = tv2bigint(tv_r);

    Bigrat *div = tv2bigrat(tv_div);
    Bigrat *trem = tv2bigrat(tv_true_rem);

    UNUSED(mp_rat_div(K, n, d, div));

    /* Now use the integral part as the quotient and the fractional part times
       the divisor as the remainder, but then correct the remainder so that it's
       always positive like in kbigint_div_and_mod */
       
    UNUSED(mp_int_div(K, MP_NUMER_P(div), MP_DENOM_P(div), q, r));

    /* NOTE: denom is positive, so div & q & r have the same sign */

    /* first adjust the quotient if necessary,
       the remainder will just fall into place after this */
    if (mp_rat_compare_zero(n) < 0)
	UNUSED(mp_int_add_value(K, q, mp_rat_compare_zero(d) < 0? 1 : -1, q));

    UNUSED(mp_rat_sub_int(K, div, q, trem));
    UNUSED(mp_rat_mul(K, trem, d, trem));

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    *res_r = kbigrat_try_integer(K, tv_true_rem);
    return kbigrat_try_integer(K, tv_q);
}

TValue kbigrat_div0_mod0(klisp_State *K, TValue n1, TValue n2, TValue *res_r)
{
    /* NOTE: quotient is always an integer, remainder may be any rational */
    TValue tv_q = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_q);
    TValue tv_r = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_r);
    /* for temp values */
    TValue tv_true_rem = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_true_rem);
    TValue tv_div = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_div);

    Bigrat *n = tv2bigrat(n1);
    Bigrat *d = tv2bigrat(n2);

    Bigint *q = tv2bigint(tv_q);
    Bigint *r = tv2bigint(tv_r);

    Bigrat *div = tv2bigrat(tv_div);
    Bigrat *trem = tv2bigrat(tv_true_rem);

    UNUSED(mp_rat_div(K, n, d, div));

    /* Now use the integral part as the quotient and the fractional part times
       the divisor as the remainder, but then correct the remainder so that it's
       in the interval [-|d/2|, |d/2|) */
       
    UNUSED(mp_int_div(K, MP_NUMER_P(div), MP_DENOM_P(div), q, r));
    /* NOTE: denom is positive, so div & q & r have the same sign */
    UNUSED(mp_rat_sub_int(K, div, q, trem));
    UNUSED(mp_rat_mul(K, trem, d, trem));

    /* NOTE: temporarily use trem as d/2 */
    TValue tv_d_2 = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_d_2);
    Bigrat *d_2 = tv2bigrat(tv_d_2);
    TValue m2 = i2tv(2);
    kensure_bigint(m2);
    UNUSED(mp_rat_div_int(K, d, tv2bigint(m2), d_2));
    /* adjust remainder and quotient if necessary */
    /* first check positive side (closed part of the interval) */
    mp_rat_abs(K, d_2, d_2);

    /* the case analysis is like in bigint (and inverse to that of fixint) */
    if (mp_rat_compare(K, trem, d_2) >= 0) {
	if (mp_rat_compare_zero(d) < 0) {
	    mp_rat_add(K, trem, d, trem);
	    mp_int_sub_value(K, q, 1, q);
	} else {
	    mp_rat_sub(K, trem, d, trem);
	    mp_int_add_value(K, q, 1, q);
	}
    } else {
	/* now check negative side (open part of the interval) */
	mp_rat_neg(K, d_2, d_2);
	if (mp_rat_compare(K, trem, d_2) < 0) {
	    if (mp_rat_compare_zero(d) < 0) {
		mp_rat_sub(K, trem, d, trem);
		mp_int_add_value(K, q, 1, q);
	    } else {
		mp_rat_add(K, trem, d, trem);
		mp_int_sub_value(K, q, 1, q);
	    }
	}
    }
	   
    krooted_tvs_pop(K); /* d/2 */

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    *res_r = kbigrat_try_integer(K, tv_true_rem);
    return kbigrat_try_integer(K, tv_q);
}


TValue kbigrat_divided(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_rat_div(K, tv2bigrat(n1), tv2bigrat(n2), tv2bigrat(res)));
    krooted_tvs_pop(K);
    return kbigrat_try_integer(K, res);
}

bool kbigrat_negativep(TValue tv_bigrat)
{
    return (mp_rat_compare_zero(tv2bigrat(tv_bigrat)) < 0);
}

bool kbigrat_positivep(TValue tv_bigrat)
{
    return (mp_rat_compare_zero(tv2bigrat(tv_bigrat)) > 0);
}

/* GC: These assume tv_bigrat is rooted */
/* needs the state to create a copy if negative */
TValue kbigrat_abs(klisp_State *K, TValue tv_bigrat)
{
    if (kbigrat_negativep(tv_bigrat)) {
	TValue copy = kbigrat_make_simple(K);
	krooted_tvs_push(K, copy);
	UNUSED(mp_rat_abs(K, tv2bigrat(tv_bigrat), tv2bigrat(copy)));
	krooted_tvs_pop(K);
	/* NOTE: this can never be an integer if the parameter was a bigrat */
	return copy;
    } else {
	return tv_bigrat;
    }
}

TValue kbigrat_numerator(klisp_State *K, TValue tv_bigrat)
{
    int32_t fnum = 0;
    Bigrat *bigrat = tv2bigrat(tv_bigrat);
    if (mp_rat_to_ints(bigrat, &fnum, NULL) == MP_OK)
	return i2tv(fnum);
    else {
	TValue copy = kbigint_make_simple(K);
	krooted_tvs_push(K, copy);
	UNUSED(mp_rat_numer(K, bigrat, tv2bigint(copy)));
	krooted_tvs_pop(K);
	/* NOTE: may still be a fixint because mp_rat_to_ints fails if
	   either numer or denom isn't a fixint */
	return kbigint_try_fixint(K, copy);
    }
}

TValue kbigrat_denominator(klisp_State *K, TValue tv_bigrat)
{
    int32_t fden = 0;
    Bigrat *bigrat = tv2bigrat(tv_bigrat);
    if (mp_rat_to_ints(bigrat, NULL, &fden) == MP_OK)
	return i2tv(fden);
    else {
	TValue copy = kbigint_make_simple(K);
	krooted_tvs_push(K, copy);
	UNUSED(mp_rat_denom(K, bigrat, tv2bigint(copy)));
	krooted_tvs_pop(K);
	/* NOTE: may still be a fixint because mp_rat_to_ints fails if
	   either numer or denom isn't a fixint */
	return kbigint_try_fixint(K, copy);
    }
}

TValue kbigrat_to_integer(klisp_State *K, TValue tv_bigrat, kround_mode mode)
{
    /* do an usigned divide first */
    TValue tv_quot = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_quot);
    TValue tv_rest = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_rest);
    Bigint *quot = tv2bigint(tv_quot);
    Bigint *rest = tv2bigint(tv_rest);
    Bigrat *n = tv2bigrat(tv_bigrat);

    UNUSED(mp_int_abs(K, MP_NUMER_P(n), quot));
    UNUSED(mp_int_div(K, quot, MP_DENOM_P(n), quot, rest));

    if (mp_rat_compare_zero(n) < 0)
	UNUSED(mp_int_neg(K, quot, quot));

    switch(mode) {
    case K_TRUNCATE: 
	/* nothing needs to be done */
	break;
    case K_CEILING:
	if (mp_rat_compare_zero(n) > 0 && mp_int_compare_zero(rest) != 0)
	    UNUSED(mp_int_add_value(K, quot, 1, quot));
	break;
    case K_FLOOR:
	if (mp_rat_compare_zero(n) < 0 && mp_int_compare_zero(rest) != 0)
	    UNUSED(mp_int_sub_value(K, quot, 1, quot));
	break;
    case K_ROUND_EVEN: {
	UNUSED(mp_int_mul_pow2(K, rest, 1, rest));
	int cmp = mp_int_compare(rest, MP_DENOM_P(n));
	if (cmp > 0 || (cmp == 0 && mp_int_is_odd(quot))) {
	    UNUSED(mp_int_add_value(K, quot, mp_rat_compare_zero(n) < 0? 
				    -1 : 1, quot));
	}
	break;
    }
    }

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, tv_quot);
}

/*
** SOURCE NOTE: this implementation is from the Haskell 98 report
*/
/*
approxRational x eps    =  simplest (x-eps) (x+eps)
        where simplest x y | y < x      =  simplest y x
                           | x == y     =  xr
                           | x > 0      =  simplest' n d n' d'
                           | y < 0      =  - simplest' (-n') d' (-n) d
                           | otherwise  =  0 :% 1
                                        where xr@(n:%d) = toRational x
                                              (n':%d')  = toRational y

              simplest' n d n' d'       -- assumes 0 < n%d < n'%d'
                        | r == 0     =  q :% 1
                        | q /= q'    =  (q+1) :% 1
                        | otherwise  =  (q*n''+d'') :% n''
                                     where (q,r)      =  quotRem n d
                                           (q',r')    =  quotRem n' d'
                                           (n'':%d'') =  simplest' d' r' d r

*/

/* 
** NOTE: this reads almost like a Haskell commercial.
** The c code is an order of magnitude longer. Some of this has to do
** with the representation of values, some because this is iterative, 
** some because of GC rooting, some because of lack of powerful pattern 
** matching, and so on, and so on
*/

/* Assumes 0 < n1/d1 < n2/d2 */
/* GC: Assumes n1, d1, n2, d2, and res are fresh (can be mutated) and rooted */
static void simplest(klisp_State *K, TValue tv_n1, TValue tv_d1, 
		     TValue tv_n2, TValue tv_d2, TValue tv_res)
{
    Bigint *n1 = tv2bigint(tv_n1);
    Bigint *d1 = tv2bigint(tv_d1);
    Bigint *n2 = tv2bigint(tv_n2);
    Bigint *d2 = tv2bigint(tv_d2);

    Bigrat *res = tv2bigrat(tv_res);

    /* create vars q1, r1, q2 & r2 */
    TValue tv_q1 = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_q1);
    Bigint *q1 = tv2bigint(tv_q1);

    TValue tv_r1 = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_r1);
    Bigint *r1 = tv2bigint(tv_r1);

    TValue tv_q2 = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_q2);
    Bigint *q2 = tv2bigint(tv_q2);

    TValue tv_r2 = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_r2);
    Bigint *r2 = tv2bigint(tv_r2);

    while(true) {
	UNUSED(mp_int_div(K, n1, d1, q1, r1));
	UNUSED(mp_int_div(K, n2, d2, q2, r2));

	if (mp_int_compare_zero(r1) == 0) {
	    /* res = q1 / 1 */
	    mp_rat_zero(K, res);
	    mp_rat_add_int(K, res, q1, res);
	    break;
	} else if (mp_int_compare(q1, q2) != 0) {
	    /* res = (q1 + 1) / 1 */
	    mp_rat_zero(K, res);
	    mp_int_add_value(K, q1, 1, q1);
	    mp_rat_add_int(K, res, q1, res);
	    break;
	} else {
	    /* simulate a recursive call */
	    TValue saved_q1 = kbigint_make_simple(K);
	    krooted_tvs_push(K, saved_q1);
	    UNUSED(mp_int_copy(K, q1, tv2bigint(saved_q1)));
	    ks_spush(K, saved_q1);
	    krooted_tvs_pop(K);

	    UNUSED(mp_int_copy(K, d2, n1));
	    UNUSED(mp_int_copy(K, d1, n2));
	    UNUSED(mp_int_copy(K, r2, d1));
	    UNUSED(mp_int_copy(K, r1, d2));
	} /* fall through */
    }

    /* now, if there were "recursive" calls, complete them */
    while(!ks_sisempty(K)) {
	TValue saved_q1 = ks_sget(K);
	TValue tv_tmp = kbigrat_make_simple(K);
	krooted_tvs_push(K, tv_tmp);
	Bigrat *tmp = tv2bigrat(tv_tmp);

	UNUSED(mp_rat_copy(K, res, tmp));
	/* res = (saved_q * n(res)) + d(res)) / n(res) */
	UNUSED(mp_rat_zero(K, res));
	UNUSED(mp_rat_add_int(K, res, tv2bigint(saved_q1), res));
	UNUSED(mp_rat_mul_int(K, res, MP_NUMER_P(tmp), res));
	UNUSED(mp_rat_add_int(K, res, MP_DENOM_P(tmp), res));
	UNUSED(mp_rat_div_int(K, res, MP_NUMER_P(tmp), res));
	krooted_tvs_pop(K); /* tmp */
	ks_sdpop(K); /* saved_q1 */
    }

    krooted_tvs_pop(K); /* q1, r1, q2, r2 */
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    return;
}

TValue kbigrat_simplest_rational(klisp_State *K, TValue tv_n1, TValue tv_n2)
{
    TValue tv_res = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_res);
    Bigrat *res = tv2bigrat(tv_res);
    Bigrat *n1 = tv2bigrat(tv_n1);
    Bigrat *n2 = tv2bigrat(tv_n2);

    int32_t cmp = mp_rat_compare(K, n1, n2);
    if (cmp > 0) { /* n1 > n2, swap */
	TValue temp = tv_n1;
	tv_n1 = tv_n2;
	tv_n2 = temp;
	n1 = tv2bigrat(tv_n1);
	n2 = tv2bigrat(tv_n2);
	/* fall through */
    } else if (cmp == 0) { /* n1 == n2 */
	krooted_tvs_pop(K);
	return tv_n1;
    } /* else fall through */

    /* we now know that n1 < n2 */
    if (mp_rat_compare_zero(n1) > 0) {
	/* 0 > n1 > n2 */
	TValue num1 = kbigint_make_simple(K);
	krooted_tvs_push(K, num1);
	UNUSED(mp_rat_numer(K, n1, tv2bigint(num1)));

	TValue den1 = kbigint_make_simple(K);
	krooted_tvs_push(K, den1);
	UNUSED(mp_rat_denom(K, n1, tv2bigint(den1)));

	TValue num2 = kbigint_make_simple(K);
	krooted_tvs_push(K, num2);
	UNUSED(mp_rat_numer(K, n2, tv2bigint(num2)));

	TValue den2 = kbigint_make_simple(K);
	krooted_tvs_push(K, den2);
	UNUSED(mp_rat_denom(K, n2, tv2bigint(den2)));

	simplest(K, num1, den1, num2, den2, tv_res);

	krooted_tvs_pop(K); /* num1, num2, den1, den2 */
	krooted_tvs_pop(K);
	krooted_tvs_pop(K);
	krooted_tvs_pop(K);

	krooted_tvs_pop(K); /* tv_res */
	return kbigrat_try_integer(K, tv_res);
    } else if (mp_rat_compare_zero(n2) < 0) {
	/* n1 < n2 < 0 */

	/* do -(simplest -n2/d2 -n1/d1) */

	TValue num1 = kbigint_make_simple(K);
	krooted_tvs_push(K, num1);
	UNUSED(mp_int_neg(K, MP_NUMER_P(n2), tv2bigint(num1)));

	TValue den1 = kbigint_make_simple(K);
	krooted_tvs_push(K, den1);
	UNUSED(mp_rat_denom(K, n2, tv2bigint(den1)));

	TValue num2 = kbigint_make_simple(K);
	krooted_tvs_push(K, num2);
	UNUSED(mp_int_neg(K, MP_NUMER_P(n1), tv2bigint(num2)));

	TValue den2 = kbigint_make_simple(K);
	krooted_tvs_push(K, den2);
	UNUSED(mp_rat_denom(K, n1, tv2bigint(den2)));

	simplest(K, num1, den1, num2, den2, tv_res);
	mp_rat_neg(K, res, res);

	krooted_tvs_pop(K); /* num1, num2, den1, den2 */
	krooted_tvs_pop(K);
	krooted_tvs_pop(K);
	krooted_tvs_pop(K);

	krooted_tvs_pop(K); /* tv_res */
	return kbigrat_try_integer(K, tv_res);
    } else {
	/* n1 < 0 < n2 */
	krooted_tvs_pop(K);
	return i2tv(0);
    }
}

TValue kbigrat_rationalize(klisp_State *K, TValue tv_n1, TValue tv_n2)
{
    /* delegate all work to simplest_rational */
    TValue tv_min = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_min);
    TValue tv_max = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_max);

    Bigrat *n1 = tv2bigrat(tv_n1);
    Bigrat *n2 = tv2bigrat(tv_n2);
    /* it doesn't matter if these are reversed */
    Bigrat *min = tv2bigrat(tv_min);
    Bigrat *max = tv2bigrat(tv_max);

    UNUSED(mp_rat_sub(K, n1, n2, min));
    UNUSED(mp_rat_add(K, n1, n2, max));

    TValue res = kbigrat_simplest_rational(K, tv_min, tv_max);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    return res;
}
