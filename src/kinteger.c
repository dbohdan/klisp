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

#define bind_iter kbind_bigint_iter
#define iter_has_next kbigint_iter_has_more
#define iter_next kbigint_iter_next
#define iter_update_last kbigint_iter_update_last

#define LOG_BASE(base) (log(2.0) / log(base))

Bigint_Node *make_new_node(klisp_State *K, uint32_t digit)
{
    Bigint_Node *node = klispM_new(K, Bigint_Node);
    node->digit = digit;
    node->next_xor_prev = (uintptr_t) 0; /* NULL ^ NULL: 0 */
    return node;
}

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

    /* GC: root bigint */
    /* put dummy value to work if garbage collections happens while allocating
       node */
    new_bigint->sign_size = 0;
    new_bigint->first = new_bigint->last = NULL;

    Bigint_Node *node = make_new_node(K, digit);
    new_bigint->first = new_bigint->last = node;
    new_bigint->sign_size = sign? -1 : 1;

    return gc2bigint(new_bigint);
}

/* used in write to destructively get the digits */
TValue kbigint_copy(klisp_State *K, TValue src)
{
    Bigint *srcb = tv2bigint(src);
    /* iterate in little endian mode */
    bind_iter(iter, srcb, false);
    uint32_t digit = iter_next(iter);
    /* GC: root copy */
    TValue copy = kbigint_new(K, kbigint_sign(srcb), digit);
    Bigint *copyb = tv2bigint(copy);

    while(iter_has_next(iter)) {
	uint32_t digit = iter_next(iter);
	kbigint_add_node(copyb, make_new_node(K, digit));
    }

    return copy;
}

/* This algorithm is like a fused multiply add on bignums,
   unlike any other function here it modifies bigint. It is used in read
   and it assumes that bigint is positive */
void kbigint_add_digit(klisp_State *K, TValue tv_bigint, int32_t base, 
		       int32_t digit)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    /* iterate in little endian mode */
    bind_iter(iter, bigint, false);

    uint64_t carry = digit;
    
    while(iter_has_next(iter)) {
	uint32_t cur = iter_next(iter);
	/* I hope the compiler understand that this should be a 
	   32bits x 32bits = 64bits multiplication instruction */
	uint64_t res_carry = (uint64_t) cur * base + carry;
	carry = res_carry >> 32;
	uint32_t new_cur = (uint32_t) res_carry;
	iter_update_last(iter, new_cur);
    }

    if (carry != 0) {
	/* must add one node to the bigint */
	kbigint_add_node(bigint, make_new_node(K, (uint32_t) carry));
    }
}

/* This is used by the writer to get the digits of a number 
 tv_bigint must be positive */
int32_t kbigint_remove_digit(klisp_State *K, TValue tv_bigint, int32_t base)
{
    assert(kbigint_has_digits(K, tv_bigint));

    Bigint *bigint = tv2bigint(tv_bigint);

    /* iterate in big endian mode */
    bind_iter(iter, bigint, true);

    uint32_t result = 0;
    uint32_t rest = 0;
    uint32_t divisor = base;

    while(iter_has_next(iter)) {
	uint64_t dividend = (((uint64_t) rest) << 32) | 
	    (uint64_t) iter_next(iter);

	if (dividend >= divisor) { /* avoid division if possible */
	    /* I hope the compiler calculates this only once and
	     is able to get that this is a 64bits by 32bits division 
	    instruction */
	    result = (uint32_t) (dividend / divisor);
	    rest = (uint32_t) (dividend % divisor);
	} else {
	    result = 0;
	    rest = (uint32_t) dividend;
	}
	iter_update_last(iter, result);
    }

    /* rest now contains the last digit & result the value of the top node */

    /* adjust the node list, at most the bigint should lose a node */
    if (bigint->first->digit == 0) {
	Bigint_Node *node = kbigint_remove_node(bigint);
	klispM_free(K, node);
    }
    
    return (int32_t) rest;
}

/* This is used by write to test if there is any digit left to print */
bool kbigint_has_digits(klisp_State *K, TValue tv_bigint)
{
    UNUSED(K);
    return kbigint_size(tv2bigint(tv_bigint)) != 0;
}

bool kbigint_negativep(TValue tv_bigint)
{
    return kbigint_negp(tv2bigint(tv_bigint));
}

/* unlike the positive? applicative this would return true on zero, 
   but zero is never represented as a bigint so there is no problem */
bool kbigint_positivep(TValue tv_bigint)
{
    return kbigint_posp(tv2bigint(tv_bigint));
}

/* Mutate the bigint to have the opposite sign, used in read */
void kbigint_invert_sign(TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    bigint->sign_size = -bigint->sign_size;
}

/* this is used by write to estimate the number of chars necessary to
   print the number */
int32_t kbigint_print_size(TValue tv_bigint, int32_t base)
{
    /* count the number of bits and estimate using the log of
       the base */
    Bigint *bigint = tv2bigint(tv_bigint);
    
    int32_t num_bits = 0;
    uint32_t first_digit = bigint->first->digit;
    while(first_digit != 0) {
	++num_bits;
	first_digit >>= 1;
    }
    num_bits += 32 * (kbigint_size(bigint)) - 2 ;
    /* add 1.5 for safety */
    return (int32_t)(LOG_BASE(base) * num_bits + 1.0); 
}
