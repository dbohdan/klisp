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

/* for now used only for reading */
/* NOTE: is uint to allow INT32_MIN as positive argument in read */
TValue kbigint_new(klisp_State *K, bool sign, uint32_t digit)
{
    Bigint *new_bigint = klispM_new(K, Bigint);

    /* header + gc_fields */
    new_bigint->next = K->root_gc;
    K->root_gc = (GCObject *)new_bigint;
    new_bigint->gct = 0;
    new_bigint->tt = K_TBIGINT;
    new_bigint->flags = 0;

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
TValue kbigint_copy(klisp_State *K, TValue src)
{
    TValue copy = kbigint_new(K, false, 0);
    Bigint *src_bigint = tv2bigint(src);
    Bigint *copy_bigint = tv2bigint(copy);
    UNUSED(mp_int_init_copy(copy_bigint, src_bigint));
    return copy;
}

/* This algorithm is like a fused multiply add on bignums,
   unlike any other function here it modifies bigint. It is used in read
   and it assumes that bigint is positive */
void kbigint_add_digit(klisp_State *K, TValue tv_bigint, int32_t base, 
		       int32_t digit)
{
    UNUSED(K);
    Bigint *bigint = tv2bigint(tv_bigint);
    UNUSED(mp_int_mul_value(bigint, base, bigint));
    UNUSED(mp_int_add_value(bigint, digit, bigint));
}

/* This is used by the writer to get the digits of a number 
 tv_bigint must be positive */
int32_t kbigint_remove_digit(klisp_State *K, TValue tv_bigint, int32_t base)
{
    UNUSED(K);
    Bigint *bigint = tv2bigint(tv_bigint);
    int32_t r;
    UNUSED(mp_int_div_value(bigint, base, bigint, &r));
    return r;
}

/* This is used by write to test if there is any digit left to print */
bool kbigint_has_digits(klisp_State *K, TValue tv_bigint)
{
    UNUSED(K);
    Bigint *bigint = tv2bigint(tv_bigint);
    return (mp_int_compare_zero(bigint) != 0);
}

/* Mutate the bigint to have the opposite sign, used in read,
   write and abs */
void kbigint_invert_sign(klisp_State *K, TValue tv_bigint)
{
    UNUSED(K);
    Bigint *bigint = tv2bigint(tv_bigint);
    UNUSED(mp_int_neg(bigint, bigint));
}

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigint_print_size(TValue tv_bigint, int32_t base)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    return mp_int_string_len(bigint, base);
}

bool kbigint_eqp(TValue tv_bigint1, TValue tv_bigint2)
{
    Bigint *bigint1 = tv2bigint(tv_bigint1);
    Bigint *bigint2 = tv2bigint(tv_bigint2);
    return (mp_int_compare(bigint1, bigint2) == 0);
}

bool kbigint_ltp(TValue tv_bigint1, TValue tv_bigint2)
{
    Bigint *bigint1 = tv2bigint(tv_bigint1);
    Bigint *bigint2 = tv2bigint(tv_bigint2);
    return (mp_int_compare(bigint1, bigint2) < 0);
}

bool kbigint_lep(TValue tv_bigint1, TValue tv_bigint2)
{
    Bigint *bigint1 = tv2bigint(tv_bigint1);
    Bigint *bigint2 = tv2bigint(tv_bigint2);
    return (mp_int_compare(bigint1, bigint2) <= 0);
}

bool kbigint_gtp(TValue tv_bigint1, TValue tv_bigint2)
{
    Bigint *bigint1 = tv2bigint(tv_bigint1);
    Bigint *bigint2 = tv2bigint(tv_bigint2);
    return (mp_int_compare(bigint1, bigint2) > 0);
}

bool kbigint_gep(TValue tv_bigint1, TValue tv_bigint2)
{
    Bigint *bigint1 = tv2bigint(tv_bigint1);
    Bigint *bigint2 = tv2bigint(tv_bigint2);
    return (mp_int_compare(bigint1, bigint2) >= 0);
}

bool kbigint_negativep(TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    return (mp_int_compare_zero(bigint) < 0);
}

bool kbigint_positivep(TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    return (mp_int_compare_zero(bigint) > 0);
}

bool kbigint_oddp(TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    return mp_int_is_odd(bigint);
}

bool kbigint_evenp(TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    return mp_int_is_even(bigint);
}

TValue kbigint_abs(klisp_State *K, TValue tv_bigint)
{
    UNUSED(K);
    Bigint *src_bigint = tv2bigint(tv_bigint);
    TValue copy = kbigint_new(K, false, 0);
    Bigint *copy_bigint = tv2bigint(copy);
    UNUSED(mp_int_abs(copy_bigint, src_bigint));
    return copy;
}
