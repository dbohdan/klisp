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
    TValue copy = kbigint_make_simple(K);
    krooted_tvs_push(K, copy);
    /* arguments are in reverse order with respect to mp_int_copy */
    UNUSED(mp_int_init_copy(K, tv2bigint(copy), tv2bigint(src)));
    krooted_tvs_pop(K);
    return copy;
}

/* 
** read/write interface 
*/

/* this works for bigints & fixints, returns true if ok */
bool kinteger_read(klisp_State *K, char *buf, int32_t base, TValue *out, 
                   char **end)
{
    TValue res = kbigint_make_simple(K);
    krooted_tvs_push(K, res);
    bool ret_val = (mp_int_read_cstring(K, tv2bigint(res), base, 
                                        buf, end) == MP_OK);
    krooted_tvs_pop(K);
    *out = kbigint_try_fixint(K, res);
    return ret_val;
}

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigint_print_size(TValue tv_bigint, int32_t base)
{
    klisp_assert(ttisbigint(tv_bigint));
    return mp_int_string_len(tv2bigint(tv_bigint), base);
}

/* this is used by write */
void  kbigint_print_string(klisp_State *K, TValue tv_bigint, int32_t base, 
                           char *buf, int32_t limit)
{
    klisp_assert(ttisbigint(tv_bigint));
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
    TValue res = kbigint_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_add(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

TValue kbigint_times(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigint_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_mul(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

TValue kbigint_minus(klisp_State *K, TValue n1, TValue n2)
{
    TValue res = kbigint_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_sub(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

/* NOTE: n2 can't be zero, that case should be checked before calling this */
TValue kbigint_div_mod(klisp_State *K, TValue n1, TValue n2, TValue *res_r)
{
    TValue tv_q = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_q);
    TValue tv_r = kbigint_make_simple(K);
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
    TValue tv_q = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_q);
    TValue tv_r = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_r);

    Bigint *n = tv2bigint(n1);
    Bigint *d = tv2bigint(n2);

    Bigint *q = tv2bigint(tv_q);
    Bigint *r = tv2bigint(tv_r);
    UNUSED(mp_int_div(K, n, d, q, r));

    /* Adjust q & r so that -|d/2| <= r < |d/2| */
    /* It seems easier to check -|d| <= 2r < |d| */
    TValue tv_two_r = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_two_r);
    Bigint *two_r = tv2bigint(tv_two_r);
    /* two_r = r * 2 = r * 2^1 */
    UNUSED(mp_int_mul_pow2(K, r, 1, two_r));
    TValue tv_abs_d = kbigint_make_simple(K);
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
        TValue copy = kbigint_make_simple(K);
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
    TValue res = kbigint_make_simple(K);
    krooted_tvs_push(K, res);
    UNUSED(mp_int_gcd(K, tv2bigint(n1), tv2bigint(n2), tv2bigint(res)));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, res);
}

TValue kbigint_lcm(klisp_State *K, TValue n1, TValue n2)
{
    TValue tv_res = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_res);
    Bigint *res = tv2bigint(tv_res);
    /* unlike in kernel, lcm in IMath can return a negative value
       (if sign a != sign b) */
    UNUSED(mp_int_lcm(K, tv2bigint(n1), tv2bigint(n2), res));
    UNUSED(mp_int_abs(K, res, res));
    krooted_tvs_pop(K);
    return kbigint_try_fixint(K, tv_res);
}

TValue kinteger_new_uint64(klisp_State *K, uint64_t x)
{
    if (x <= INT32_MAX) {
        return i2tv((int32_t) x);
    } else {
        TValue res = kbigint_make_simple(K);
        krooted_tvs_push(K, res);

        uint8_t d[8];
        for (int i = 7; i >= 0; i--) {
            d[i] = (x & 0xFF);
            x >>= 8;
        }

        mp_int_read_unsigned(K, tv2bigint(res), d, 8);
        krooted_tvs_pop(K);
        return res;
    }
}
