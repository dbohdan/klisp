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

/* for now used only for reading */
/* NOTE: is uint and has flag to allow INT32_MIN as positive argument */
TValue kbigint_new(klisp_State *K, bool sign, uint32_t digit);

/* used in write to destructively get the digits */
TValue kbigint_copy(klisp_State *K, TValue src);

/* Create a stack allocated bigints from a fixint,
   useful for mixed operations, relatively light weight compared
   to creating it in the heap and burdening the gc */
#define kbind_bigint(name, fixint)					\
    int32_t (KUNIQUE_NAME(i)) = fixint;					\
    BigintNode KUNIQUE_NAME(node);					\
    node.val = { int64_t temp = (KUNIQUE_NAME(i));			\
		 (uint32_t) (temp < 0)? -temp : temp; };		\
    node.next_xor_prev = (uintptr_t) 0;	/* NULL ^ NULL: 0 */		\
    Bigint KUNIQUE_NAME(bigint);					\
    (KUNIQUE_NAME(bigint)).first = &(KUNIQUE_NAME(node));		\
    (KUNIQUE_NAME(bigint)).last = &(KUNIQUE_NAME(node));		\
    (KUNIQUE_NAME(bigint)).sign_size = (KUNIQUE_NAME(i)) < 0? -1 : 1;	\
    Bigint *name = &(KUNIQUE_NAME(bigint));
    
/* This is used by the reader to destructively add digits to a number 
 tv_bigint must be positive */
void kbigint_add_digit(klisp_State *K, TValue tv_bigint, int32_t base, 
		       int32_t digit);

/* This is used by the writer to get the digits of a number 
 tv_bigint must be positive */
int32_t kbigint_remove_digit(klisp_State *K, TValue tv_bigint, int32_t base);

/* This is used by write to test if there is any digit left to print */
bool kbigint_has_digits(klisp_State *K, TValue tv_bigint);

bool kbigint_negativep(TValue tv_bigint);
bool kbigint_positivep(TValue tv_bigint);
/* Mutate the bigint to have the opposite sign, used in read & write */
void kbigint_invert_sign(TValue tv_bigint);

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigint_print_size(TValue tv_bigint, int32_t base);

#endif
