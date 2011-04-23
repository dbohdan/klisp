/*
** kinteger.c
** Kernel Integers (fixints and bigints)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* This tries to convert a bigint to a fixint */
inline TValue kbigint_try_fixint(klisp_State *K, TValue n)
{
    Bigint *b = tv2bigint(n);
    if (MP_USED(b) != 1)
	return n;

    int64_t digit = (int64_t) *(MP_DIGITS(b));
    if (MP_SIGN(b) == MP_NEG) digit = -digit;
    if (kfit_int32_t(digit)) {
	/* n shouln't be reachable but the let the gc do its job */
	return i2tv((int32_t) digit); 
    } else {
	return n;
    }
}

/* It is used for reading and for creating temps and res in all operations */
/* NOTE: is uint to allow INT32_MIN as positive argument in read */
TValue kbigint_new(klisp_State *K, bool sign, uint32_t digit)
{
    Bigint *new_bigint = klispM_new(K, Bigint);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_bigint, K_TBIGINT, 0);

    /* bigint specific fields */
    /* If later changed to alloc obj: 
       GC: root bigint & put dummy value to work if garbage collections 
       happens while allocating array */
    new_bigint->single = digit;
    new_bigint->digits = &(new_bigint->single);
    new_bigint->alloc = 1;
    new_bigint->used = 1;
    new_bigint->sign = sign? MP_NEG : MP_ZPOS;

    return gc2bigint(new_bigint);
}

/* used in write to destructively get the digits */
/* assumes src is rooted */
TValue kbigint_copy(klisp_State *K, TValue src)
{
    TValue copy = kbigint_new(K, false, 0);
    /* arguments are in reverse order with respect to mp_int_copy */
    UNUSED(mp_int_init_copy(K, tv2bigint(copy), tv2bigint(src)));
    return copy;
}

/* This algorithm is like a fused multiply add on bignums,
   unlike any other function here it modifies bigint. It is used in read
   and it assumes that bigint is positive */
/* GC: Assumes tv_bigint is rooted */
void kbigint_add_digit(klisp_State *K, TValue tv_bigint, int32_t base, 
		       int32_t digit)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    UNUSED(mp_int_mul_value(K, bigint, base, bigint));
    UNUSED(mp_int_add_value(K, bigint, digit, bigint));
}

/* This is used by the writer to get the digits of a number 
 tv_bigint must be positive */
/* GC: Assumes tv_bigint is rooted */
int32_t kbigint_remove_digit(klisp_State *K, TValue tv_bigint, int32_t base)
{
    UNUSED(K);
    Bigint *bigint = tv2bigint(tv_bigint);
    int32_t r;
    UNUSED(mp_int_div_value(K, bigint, base, bigint, &r));
    return r;
}

/* This is used by write to test if there is any digit left to print */
bool kbigint_has_digits(klisp_State *K, TValue tv_bigint)
{
    UNUSED(K);
    return (mp_int_compare_zero(tv2bigint(tv_bigint)) != 0);
}

/* Mutate the bigint to have the opposite sign, used in read
   and write*/
/* GC: Assumes tv_bigint is rooted */
void kbigint_invert_sign(klisp_State *K, TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    UNUSED(mp_int_neg(K, bigint, bigint));
}

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigint_print_size(TValue tv_bigint, int32_t base)
{
    return mp_int_string_len(tv2bigint(tv_bigint), base);
}

/* this is used by write */
void  kbigint_print_string(klisp_State *K, TValue tv_bigint, int32_t base, 
			   char *buf, int32_t limit)
{
    mp_result res = mp_int_to_string(K, tv2bigint(tv_bigint), base, buf, 
				     limit);
    /* only possible error is truncation */
    klisp_assert(res == MP_OK);
}

/* Interface for kgnumbers */
bool kbigint_eqp(TValue tv_bigint1, TValue tv_bigint2)
{
    return (mp_int_compare(tv2bigint(tv_bigint1), 
			   tv2bigint(tv_bigint2)) == 0);
}

bool kbigint_ltp(TValue tv_bigint1, TValue tv_bigint2)
{
    return (mp_int_compare(tv2bigint(tv_bigint1), 
			   tv2bigint(tv_bigint2)) < 0);
}

bool kbigint_lep(TValue tv_bigint1, TValue tv_bigint2)
{
    return (mp_int_compare(tv2bigint(tv_bigint1), 
			   tv2bigint(tv_bigint2)) <= 0);
}

bool kbigint_gtp(TValue tv_bigint1, TValue tv_bigint2)
{
    return (mp_int_compare(tv2bigint(tv_bigint1), 
			   tv2bigint(tv_bigint2)) > 0);
}

bool kbigint_gep(TValue tv_bigint1, TValue tv_bigint2)
{
    return (mp_int_compare(tv2bigint(tv_bigint1), 
			   tv2bigint(tv_bigint2)) >= 0);
}

/*
** GC: All of these assume the parameters are rooted 
*/
TValue kbigint_plus(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigint_new(K, false, 0);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_add(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

TValue kbigint_times(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigint_new(K, false, 0);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_mul(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

TValue kbigint_minus(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigint_new(K, false, 0);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_sub(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

/* NOTE: n2 can't be zero, that case should be checked before calling this */
TValue kbigint_div_mod(klisp_State *K, TValue n1, TValue n2, TValue *res_r)
{
    TValue tv_q = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_q);
    TValue tv_r = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_r);

    Bigint *n = tv2bigint(n1);
    Bigint *d = tv2bigint(n2);

    Bigint *q = tv2bigint(tv_q);
    Bigint *r = tv2bigint(tv_r);

    UNUSED(mp_int_div(K, n, d, q, r));

    /* Adjust q & r so that 0 <= r < |d| */
    if (mp_int_compare_zero(r) < 0) {
	if (mp_int_compare_zero(d) < 0) {
	    mp_int_sub(K, r, d, r);
	    mp_int_add_value(K, q, 1, q);
	} else {
	    mp_int_add(K, r, d, r);
	    mp_int_sub_value(K, q, 1, q);
	}
    }

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    *res_r = kbigint_try_fixint(K, tv_r);
    return kbigint_try_fixint(K, tv_q);
}

TValue kbigint_div0_mod0(klisp_State *K, TValue n1, TValue n2, TValue *res_r)
{
    /* GC: root bigints */
    TValue tv_q = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_q);
    TValue tv_r = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_r);

    Bigint *n = tv2bigint(n1);
    Bigint *d = tv2bigint(n2);

    Bigint *q = tv2bigint(tv_q);
    Bigint *r = tv2bigint(tv_r);
    UNUSED(mp_int_div(K, n, d, q, r));

    /* Adjust q & r so that -|d/2| <= r < |d/2| */
    /* It seems easier to check -|d| <= 2r < |d| */
    TValue tv_two_r = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_two_r);
    Bigint *two_r = tv2bigint(tv_two_r);
    /* two_r = r * 2 = r * 2^1 */
    UNUSED(mp_int_mul_pow2(K, r, 1, two_r));
    TValue tv_abs_d = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_abs_d);
    /* NOTE: this makes a copy if d >= 0 */
    Bigint *abs_d = tv2bigint(tv_abs_d);
    UNUSED(mp_int_abs(K, d, abs_d));
    
    /* the case analysis is inverse to that of fixint */

    /* this checks 2r >= |d| (which is the same r >= |d/2|) */
    if (mp_int_compare(two_r, abs_d) >= 0) {
	if (mp_int_compare_zero(d) < 0) {
	    mp_int_add(K, r, d, r);
	    mp_int_sub_value(K, q, 1, q);
	} else {
	    mp_int_sub(K, r, d, r);
	    mp_int_add_value(K, q, 1, q);
	}
    } else {
	UNUSED(mp_int_neg(K, abs_d, abs_d));
	/* this checks 2r < -|d| (which is the same r < |d/2|) */
	if (mp_int_compare(two_r, abs_d) < 0) {
	    if (mp_int_compare_zero(d) < 0) {
		mp_int_sub(K, r, d, r);
		mp_int_add_value(K, q, 1, q);
	    } else {
		mp_int_add(K, r, d, r);
		mp_int_sub_value(K, q, 1, q);
	    }
	}
    }

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    *res_r = kbigint_try_fixint(K, tv_r);
    return kbigint_try_fixint(K, tv_q);
}

bool kbigint_negativep(TValue tv_bigint)
{
    return (mp_int_compare_zero(tv2bigint(tv_bigint)) < 0);
}

bool kbigint_positivep(TValue tv_bigint)
{
    return (mp_int_compare_zero(tv2bigint(tv_bigint)) > 0);
}

bool kbigint_oddp(TValue tv_bigint)
{
    return mp_int_is_odd(tv2bigint(tv_bigint));
}

bool kbigint_evenp(TValue tv_bigint)
{
    return mp_int_is_even(tv2bigint(tv_bigint));
}

TValue kbigint_abs(klisp_State *K, TValue tv_bigint)
{
    if (kbigint_negativep(tv_bigint)) {
	TValue copy = kbigint_new(K, false, 0);
	krooted_tvs_push(K, copy);
	UNUSED(mp_int_abs(K, tv2bigint(tv_bigint), tv2bigint(copy)));
	krooted_tvs_pop(K);
	/* NOTE: this can never be a fixint if the parameter was a bigint */
	return copy;
    } else {
	return tv_bigint;
    }
}

TValue kbigint_gcd(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigint_new(K, false, 0);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_gcd(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

TValue kbigint_lcm(klisp_State *K, TValue n1, TValue n2)
{
    TValue tv_res = kbigint_new(K, false, 0);
    krooted_tvs_push(K, tv_res);
    Bigint *res = tv2bigint(tv_res);
    /* unlike in kernel, lcm in IMath can return a negative value
       (if sign a != sign b) */
    UNUSED(mp_int_lcm(K, tv2bigint(n1), tv2bigint(n2), res));
    UNUSED(mp_int_abs(K, res, res));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, tv_res);
}
