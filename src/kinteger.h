/*
** kinteger.h
** Kernel Integers (fixints and bigints)
** See Copyright Notice in klisp.h
*/

#ifndef kinteger_h
#define kinteger_h

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "kobject.h"
#include "kstate.h"
#include "imath.h"

/* Check to see if an int64_t fits in a int32_t */
inline bool kfit_int32_t(int64_t n) {
    return (n >= (int64_t) INT32_MIN && n <= (int64_t) INT32_MAX);
}

/* This tries to convert a bigint to a fixint */
/* XXX this doesn't need K really */
inline TValue kbigint_try_fixint(klisp_State *K, TValue n)
{
    UNUSED(K);
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

/* NOTE: is uint and has flag to allow INT32_MIN as positive argument */
TValue kbigint_new(klisp_State *K, bool sign, uint32_t digit);

TValue kbigint_copy(klisp_State *K, TValue src);

/* macro to create the simplest bigint */
#define kbigint_make_simple(K_) kbigint_new(K_, false, 0)

/* Create a stack allocated bigints from a fixint,
   useful for mixed operations, relatively light weight compared
   to creating it in the heap and burdening the gc */
#define kbind_bigint(name, fixint)					\
    int32_t (KUNIQUE_NAME(i)) = ivalue(fixint);				\
    Bigint KUNIQUE_NAME(bigint);					\
    (KUNIQUE_NAME(bigint)).single = ({					\
	    int64_t temp = (KUNIQUE_NAME(i));				\
	    (uint32_t) ((temp < 0)? -temp : temp);			\
	});								\
    (KUNIQUE_NAME(bigint)).digits = &((KUNIQUE_NAME(bigint)).single);	\
    (KUNIQUE_NAME(bigint)).alloc = 1;					\
    (KUNIQUE_NAME(bigint)).used = 1;					\
    (KUNIQUE_NAME(bigint)).sign = (KUNIQUE_NAME(i)) < 0?		\
	MP_NEG : MP_ZPOS;						\
    Bigint *name = &(KUNIQUE_NAME(bigint))
    
/* This can be used prior to calling a bigint functions
   to automatically convert fixints to bigints.
   NOTE: calls to this macro should go in different lines! */
#define kensure_bigint(n)						\
    /* must use goto, no block should be entered before calling		\
       kbind_bigint */							\
    if (!ttisfixint(n))							\
	goto KUNIQUE_NAME(exit_lbl);					\
    kbind_bigint(KUNIQUE_NAME(bint), (n));				\
    (n) = gc2bigint(KUNIQUE_NAME(bint));				\
    KUNIQUE_NAME(exit_lbl):

/* This is used by the reader to destructively add digits to a number 
 tv_bigint must be positive */
void kbigint_add_digit(klisp_State *K, TValue tv_bigint, int32_t base, 
		       int32_t digit);

/* This is used by the writer to get the digits of a number 
 tv_bigint must be positive */
int32_t kbigint_remove_digit(klisp_State *K, TValue tv_bigint, int32_t base);

/* This is used by write to test if there is any digit left to print */
bool kbigint_has_digits(klisp_State *K, TValue tv_bigint);

/* Mutate the bigint to have the opposite sign, used in read & write */
void kbigint_invert_sign(klisp_State *K, TValue tv_bigint);

/* read/write interface */

/* this works for bigints & fixints, returns true if ok */
bool kinteger_read(klisp_State *K, char *buf, int32_t base, TValue *out, 
		   char **end);

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigint_print_size(TValue tv_bigint, int32_t base);

/* this is used by write */
void  kbigint_print_string(klisp_State *K, TValue tv_bigint, int32_t base, 
			   char *buf, int32_t limit);

/* Interface for kgnumbers */
bool kbigint_eqp(TValue bigint1, TValue bigint2);

bool kbigint_ltp(TValue bigint1, TValue bigint2);
bool kbigint_lep(TValue bigint1, TValue bigint2);
bool kbigint_gtp(TValue bigint1, TValue bigint2);
bool kbigint_gep(TValue bigint1, TValue bigint2);

TValue kbigint_plus(klisp_State *K, TValue n1, TValue n2);
TValue kbigint_times(klisp_State *K, TValue n1, TValue n2);
TValue kbigint_minus(klisp_State *K, TValue n1, TValue n2);

TValue kbigint_div_mod(klisp_State *K, TValue n1, TValue n2, TValue *res_r);
TValue kbigint_div0_mod0(klisp_State *K, TValue n1, TValue n2, TValue *res_r);

bool kbigint_negativep(TValue tv_bigint);
bool kbigint_positivep(TValue tv_bigint);

bool kbigint_oddp(TValue tv_bigint);
bool kbigint_evenp(TValue tv_bigint);

/* needs the state to create a copy if negative */
TValue kbigint_abs(klisp_State *K, TValue tv_bigint);

TValue kbigint_gcd(klisp_State *K, TValue n1, TValue n2);
TValue kbigint_lcm(klisp_State *K, TValue n1, TValue n2);

#endif
