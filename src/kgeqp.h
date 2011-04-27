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
	if (ttisapplicative(obj1)) {
	    while(ttisapplicative(obj1) && ttisapplicative(obj2)) {
		obj1 = kunwrap(obj1);
		obj2 = kunwrap(obj2);
	    }
	    res = (tv_equal(obj1, obj2));
	} else if (ttisbigint(obj1)) {
	    /* it's important to know that it can't be the case
	       that obj1 is bigint and obj is some other type and
	       (eq? obj1 obj2) */
	    res = kbigint_eqp(obj1, obj2);
	} else if (ttisbigrat(obj1)) {
	    /* it's important to know that it can't be the case
	       that obj1 is bigrat and obj is some other type and
	       (eq? obj1 obj2) */
	    res = kbigrat_eqp(K, obj1, obj2);
	} /* immutable strings are interned so are covered already */
    }
    return res;
}

#endif
