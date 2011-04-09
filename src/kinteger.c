/*
** kinteger.c
** Kernel Integers (fixints and bigints)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"

#define bind_iter kbind_bigint_iter
#define iter_has_next kbigint_iter_has_more
#define iter_next kbigint_iter_next
#define iter_update_last kbigint_iter_update_last

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

/* Mutate the bigint to have the opposite sign, used in read */
void kbigint_invert_sign(TValue tv_bigint)
{
    Bigint *bigint = tv2bigint(tv_bigint);
    bigint->sign_size = -bigint->sign_size;
}
