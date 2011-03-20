/*
** kgnumbers.c
** Numbers features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgnumbers.h"

/* 15.5.1? number?, finite?, integer? */
/* use ftypep & ftypep_predp */

/* Helpers for typed predicates */
bool knumberp(TValue obj) { return ttype(obj) <= K_LAST_NUMBER_TYPE; }
/* obj is known to be a number */
bool kfinitep(TValue obj) { return (!ttiseinf(obj) && !ttisiinf(obj)); }
/* TEMP: for now only fixint, should also include bigints and 
   inexact integers */
bool kintegerp(TValue obj) { return ttisfixint(obj); }

/* 12.5.2 =? */
/* uses typed_bpredp */

/* 12.5.3 <?, <=?, >?, >=? */
/* use typed_bpredp */

/* Helpers for typed binary predicates */
/* XXX: this should probably be in a file knumber.h but there is no real need for 
   that file yet */

/* this will come handy when there are more numeric types,
   it is intended to be used in switch */
inline int32_t max_ttype(TValue obj1, TValue obj2)
{
    int32_t t1 = ttype(obj1);
    int32_t t2 = ttype(obj2);

    return (t1 > t2? t1 : t2);
}

/* TEMP: for now only fixints and exact infinities */
bool knum_eqp(TValue n1, TValue n2) { return tv_equal(n1, n2); }
bool knum_ltp(TValue n1, TValue n2) 
{ 
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT:
	return ivalue(n1) < ivalue(n2);
    case K_TEINF:
	return !tv_equal(n1, n2) && (tv_equal(n1, KEMINF) ||
				     tv_equal(n2, KEPINF));
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool knum_lep(TValue n1, TValue n2)
{ 
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT:
	return ivalue(n1) <= ivalue(n2);
    case K_TEINF:
	return tv_equal(n1, n2) || tv_equal(n1, KEMINF) || 
	    tv_equal(n2, KEPINF);
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool knum_gtp(TValue n1, TValue n2)
{ 
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT:
	return ivalue(n1) > ivalue(n2);
    case K_TEINF:
	return !tv_equal(n1, n2) && (tv_equal(n1, KEPINF) ||
				     tv_equal(n2, KEMINF));
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool knum_gep(TValue n1, TValue n2)
{ 
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT:
	return ivalue(n1) >= ivalue(n2);
    case K_TEINF:
	return tv_equal(n1, n2) || tv_equal(n1, KEPINF) ||
	    tv_equal(n2, KEMINF);
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}



