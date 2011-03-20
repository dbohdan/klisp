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

/* 12.5.4 + */
/* TEMP: for now only accept two arguments */
void kplus(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    bind_2tp(K, "+", ptree, "number", knumberp, n1, "number", knumberp, n2);

    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int32_t i1 = ivalue(n1);
	int32_t i2 = ivalue(n2);
	/* TODO: check for overflow and create bigint */
	kapply_cc(K, i2tv(i1+i2));
    } 
    case K_TEINF: {
	if (ttiseinf(n1) && ttiseinf(n2)) {
	    if (tv_equal(n1, n2)) {
		kapply_cc(K, n1);
	    } else {
		/* TEMP: we don't have reals with no prim value yet */
		/* also no strict arithmetic variable for now */
		klispE_throw(K, "+: result has no primary value");
		return;
	    }
	} else {
	    kapply_cc(K, ttiseinf(n1)? n1 : n2);
	}
    }
    default:
	/* shouldn't happen */
	assert(0);
	return;
    }
}


/* 12.5.5 * */
/* TEMP: for now only accept two arguments */
void ktimes(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    bind_2tp(K, "*", ptree, "number", knumberp, n1, "number", knumberp, n2);

    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int32_t i1 = ivalue(n1);
	int32_t i2 = ivalue(n2);
	/* TODO: check for overflow and create bigint */
	kapply_cc(K, i2tv(i1*i2));
    } 
    case K_TEINF: {
	if (kfast_zerop(n1) || kfast_zerop(n2)) {
	    /* TEMP: we don't have reals with no prim value yet */
	    /* also no strict arithmetic variable for now */
	    klispE_throw(K, "*: result has no primary value");
	    return;
	} else {
	    /* use the fact that infinities have ivalues 1 & -1 */
	    kapply_cc(K, (ivalue(n1) ^ ivalue(n2)) < 0? KEMINF : KEPINF);
	}
    }
    default:
	/* shouldn't happen */
	assert(0);
	return;
    }
}

/* 12.5.6 - */
/* TEMP: for now only accept two arguments */
void kminus(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    bind_2tp(K, "-", ptree, "number", knumberp, n1, "number", knumberp, n2);

    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int32_t i1 = ivalue(n1);
	int32_t i2 = ivalue(n2);
	/* TODO: check for overflow and create bigint */
	kapply_cc(K, i2tv(i1-i2));
    } 
    case K_TEINF: {
	if (ttiseinf(n1) && ttiseinf(n2)) {
	    if (tv_equal(n1, n2)) {
		/* TEMP: we don't have reals with no prim value yet */
		/* also no strict arithmetic variable for now */
		klispE_throw(K, "-: result has no primary value");
		return;
	    } else {
		kapply_cc(K, n1);
	    }
	} else {
	    kapply_cc(K, ttiseinf(n1)? n1 : kneg_inf(n2));
	}
    }
    default:
	/* shouldn't happen */
	assert(0);
	return;
    }
}

/* 12.5.7 zero? */
/* uses ftyped_predp */

/* Helper for zero? */
bool kzerop(TValue n) { return kfast_zerop(n); }
