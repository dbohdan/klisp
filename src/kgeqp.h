/*
** kgeqp.c
** Equivalence under mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgeqp_h
#define kgeqp_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kapplicative.h" /* for unwrap */
#include "kinteger.h" /* for kbigint_eqp */
#include "krational.h" /* for kbigrat_eqp */
#include "klisp.h"
#include "kghelpers.h"

/* 4.2.1 eq? */
/* 6.5.1 eq? */
void eqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper (also used in equal?) */
inline bool eq2p(klisp_State *K, TValue obj1, TValue obj2)
{
    bool res = (tv_equal(obj1, obj2));
    if (!res && (ttype(obj1) == ttype(obj2))) {
	switch (ttype(obj1)) {
	case K_TSYMBOL:
            /* symbols can't be compared with tv_equal! */
	    res = tv_sym_equal(obj1, obj2);
	    break;
	case K_TAPPLICATIVE:
	    while(ttisapplicative(obj1) && ttisapplicative(obj2)) {
		obj1 = kunwrap(obj1);
		obj2 = kunwrap(obj2);
	    }
	    res = (tv_equal(obj1, obj2));
	    break;
	case K_TBIGINT:
	    /* it's important to know that it can't be the case
	       that obj1 is bigint and obj is some other type and
	       (eq? obj1 obj2) */
	    res = kbigint_eqp(obj1, obj2);
	    break;
	case K_TBIGRAT:
	    /* it's important to know that it can't be the case
	       that obj1 is bigrat and obj is some other type and
	       (eq? obj1 obj2) */
	    res = kbigrat_eqp(K, obj1, obj2);
	    break;
	} /* immutable strings & bytevectors are interned so they are 
	     covered already by tv_equalp */

    }
    return res;
}

/* init ground */
void kinit_eqp_ground_env(klisp_State *K);

#endif
