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
#include "ksymbol.h"

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

/* 12.5.8 div, mod, div-and-mod */
/* use div_mod */

/* 12.5.9 div0, mod0, div0-and-mod0 */
/* use div_mod */

/* Helpers for div, mod, div0 and mod0 */

/* zerop signals div0 and mod0 if true div and mod if false */
inline void div_mod(bool zerop, int32_t n, int32_t d, int32_t *div, 
		    int32_t *mod) 
{
    *div = n / d;
    *mod = n % d;

    if (zerop) {
	/* div0, mod0 or div-and-mod0 */
	/* -|d/2| <= mod0 < |d/2| */

	int32_t dabs = ((d<0? -d : d) + 1) / 2;
	
	if (*mod < -dabs) {
	    if (d < 0) {
		*mod -= d;
		++(*div);
	    } else {
		*mod += d;
		--(*div);
	    }
	} else if (*mod >= dabs) {
	    if (d < 0) {
		*mod += d;
		--(*div);
	    } else {
		*mod -= d;
		++(*div);
	    }
	}
    } else {
	/* div, mod or div-and-mod */
	/* 0 <= mod0 < |d| */
	if (*mod < 0) {
	    if (d < 0) {
		*mod -= d;
		++(*div);
	    } else {
		*mod += d;
		--(*div);
	    }
	}
    }
}

/* flags are FDIV_DIV, FDIV_MOD, FDIV_ZERO */
void kdiv_mod(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: div_mod_flags
    */
    char *name = ksymbol_buf(xparams[0]);
    int32_t flags = ivalue(xparams[1]);

    UNUSED(denv);

    bind_2tp(K, name, ptree, "number", knumberp, tv_n,
	     "number", knumberp, tv_d);

    /* TEMP: only fixnums */
    TValue tv_div, tv_mod;

    if (kfast_zerop(tv_d)) {
	klispE_throw_extra(K, name, ": division by zero");
	return;
    } else if (ttiseinf(tv_n)) {
	klispE_throw_extra(K, name, ": non finite dividend");
	return;
    } else if (ttiseinf(tv_d)) {
	tv_div = ((ivalue(tv_n) ^ ivalue(tv_d)) < 0)? KEPINF : KEMINF;
	tv_mod = i2tv(0);
    } else {
	int32_t div, mod;
	div_mod(flags & FDIV_ZERO, ivalue(tv_n), ivalue(tv_d), &div, &mod);
	tv_div = i2tv(div);
	tv_mod = i2tv(mod);
    }
    TValue res;
    if (flags & FDIV_DIV) {
	if (flags & FDIV_MOD) { /* return both div and mod */
	    res =  kcons(K, tv_div, kcons(K, tv_mod, KNIL));
	} else {
	    res = tv_div;
	}
    } else {
	res = tv_mod;
    }
    kapply_cc(K, res);
}

/* 12.5.10 positive?, negative? */
/* use ftyped_predp */

/* 12.5.11 odd?, even? */
/* use ftyped_predp */

/* Helpers for positive?, negative?, odd? & even? */
bool kpositivep(TValue n) { return ivalue(n) > 0; }
bool knegativep(TValue n) { return ivalue(n) < 0; }
bool koddp(TValue n) { return (ivalue(n) & 1) != 0; }
bool kevenp(TValue n) { return (ivalue(n) & 1) == 0; }

/* 12.5.12 abs */
void kabs(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, "abs", ptree, "number", knumberp, n);

    switch(ttype(n)) {
    case K_TFIXINT: {
	int32_t i = ivalue(n);
	kapply_cc(K, i < 0? i2tv(-i) : n);
    }
    case K_TEINF:
	kapply_cc(K, KEPINF);
    default:
	/* shouldn't happen */
	assert(0);
	return;
    }
}

/* 12.5.13 min, max */
/* NOTE: this does two passes, one for error checking and one for doing
 the actual work */
void kmin_max(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: bool: true min, false max
    */
    UNUSED(denv);
    
    char *name = ksymbol_buf(xparams[0]);
    bool minp = bvalue(xparams[1]);

    /* cycles are allowed, loop counting pairs */
    int32_t pairs = check_typed_list(K, name, "number", knumberp, true, ptree);
    
    TValue res;
    bool one_finite = false;
    TValue break_val;
    if (minp) {
	res = KEPINF;
	break_val = KEMINF; /* min possible number */
    } else {
	res = KEMINF;
	break_val = KEPINF; /* max possible number */
    }

    TValue tail = ptree;
    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);

	if (ttiseinf(first)) {
	    if (tv_equal(first, break_val)) {
		res = first;
		break;
	    }
	} else if (!one_finite) {
	    res = first;
	    one_finite = true;
	} else if (minp) {
	    if (ivalue(first) < ivalue(res))
		res = first;
	} else { /* maxp */
	    if (ivalue(first) > ivalue(res))
		res = first;
	}
    }
    kapply_cc(K, res);
}

