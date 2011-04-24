/*
** krational.c
** Kernel Rationals (fixrats and bigrats)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h> /* for code checking '/' & '.' */
#include <inttypes.h>
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
    /* detect sign after '/', this is allowed by imrat but not in kernel */
    if (ret_val) {
	char *slash = strchr(buf, '/');
	if (slash != NULL && (*(slash+1) == '+' || *(slash+1) == '-'))
	    ret_val = false;
    }

    return ret_val;
}

/* NOTE: allow decimal for use after #e */
bool krational_read_decimal(klisp_State *K, char *buf, int32_t base, TValue *out, 
			    char **end)
{
    TValue res = kbigrat_make_simple(K);
    krooted_tvs_push(K, res);
    bool ret_val = (mp_rat_read_ustring(K, tv2bigrat(res), base, 
					buf, end) == MP_OK);
    krooted_tvs_pop(K);
    *out = kbigrat_try_integer(K, res);

    /* TODO: ideally this should be incorporated in the read code */
    /* detect sign after '/', or trailing '.'. Those are allowed by 
       imrat but not by kernel */
    if (ret_val) {
	char *ch = strchr(buf, '/');
	if (ch != NULL && (*(ch+1) == '+' || *(ch+1) == '-'))
	    ret_val = false;
	else {
	    ch = strchr(buf, '.');
	    if (ch != NULL && *(ch+1) == '\0')
		ret_val = false;
	}
    }

    return ret_val;
}

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigrat_print_size(TValue tv_bigint, int32_t base)
{
    return mp_rat_string_len(tv2bigrat(tv_bigint), base);
}

/* this is used by write */
void  kbigrat_print_string(klisp_State *K, TValue tv_bigrat, int32_t base, 
			   char *buf, int32_t limit)
{
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
