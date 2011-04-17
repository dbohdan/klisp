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
#include "kinteger.h"

#include "kghelpers.h"
#include "kgnumbers.h"

/* 15.5.1? number?, finite?, integer? */
/* use ftypep & ftypep_predp */

/* Helpers for typed predicates */
bool knumberp(TValue obj) { return ttype(obj) <= K_LAST_NUMBER_TYPE; }
/* This is used in gcd & lcm */
bool kimp_intp(TValue obj) { return ttisinteger(obj) || ttiseinf(obj); }
/* obj is known to be a number */
bool kfinitep(TValue obj) { return (!ttiseinf(obj) && !ttisiinf(obj)); }
/* TEMP: for now only fixint & bigints, should also include inexact 
   integers */
bool kintegerp(TValue obj) { return ttisinteger(obj); }

/* 12.5.2 =? */
/* uses typed_bpredp */

/* 12.5.3 <?, <=?, >?, >=? */
/* use typed_bpredp */

/* Helpers for typed binary predicates */
/* XXX: this should probably be in a file knumber.h but there is no real need for 
   that file yet */

/* this will come handy when there are more numeric types,
   it is intended to be used in switch */
/* MAYBE: change to return -1, 0, 1 to indicate which type is bigger, and
   return min & max in two extra pointers passed in. Change name to
   classify_types */
inline int32_t max_ttype(TValue obj1, TValue obj2)
{
    int32_t t1 = ttype(obj1);
    int32_t t2 = ttype(obj2);

    return (t1 > t2? t1 : t2);
}

inline int32_t min_ttype(TValue obj1, TValue obj2)
{
    int32_t t1 = ttype(obj1);
    int32_t t2 = ttype(obj2);

    return (t1 < t2? t1 : t2);
}

/* TEMP: for now only fixints, bigints and exact infinities */
bool knum_eqp(TValue n1, TValue n2) 
{ 
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT:
	return ivalue(n1) == ivalue(n2);
    case K_TBIGINT:
	if (min_ttype(n1, n2) != K_TBIGINT) {
	    /* NOTE: no fixint is =? to a bigint */
	    return false;
	} else {
	    /* both are bigints */
	    return kbigint_eqp(n1, n2);
	}
    case K_TEINF:
	return (tv_equal(n1, n2));
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool knum_ltp(TValue n1, TValue n2) 
{ 
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT:
	return ivalue(n1) < ivalue(n2);
    case K_TBIGINT: {
	kensure_bigint(n1);
	kensure_bigint(n2);
	return kbigint_ltp(n1, n2);
    }
    case K_TEINF:
	return !tv_equal(n1, n2) && (tv_equal(n1, KEMINF) ||
				     tv_equal(n2, KEPINF));
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool knum_lep(TValue n1, TValue n2) { return !knum_ltp(n2, n1); }
bool knum_gtp(TValue n1, TValue n2) { return knum_ltp(n2, n1); }
bool knum_gep(TValue n1, TValue n2) { return !knum_ltp(n1, n2); }

/* REFACTOR/MAYBE: add small inlineable plus that
   first tries fixint addition and if that fails calls knum_plus */

/* May throw an error */
/* GC: assumes n1 & n2 rooted */
TValue knum_plus(klisp_State *K, TValue n1, TValue n2)
{
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int64_t res = (int64_t) ivalue(n1) + (int64_t) ivalue(n2);
	if (res >= (int64_t) INT32_MIN &&
	    res <= (int64_t) INT32_MAX) {
	    return i2tv((int32_t) res);
	} /* else fall through */
    }
    case K_TBIGINT: {
	kensure_bigint(n1);
	kensure_bigint(n2);
	return kbigint_plus(K, n1, n2);
    }
    case K_TEINF:
	if (!ttiseinf(n1))
	    return n2;
	else if (!ttiseinf(n2))
	    return n1;
	if (tv_equal(n1, n2))
	    return n1;
	else {
	    klispE_throw(K, "+: no primary value");
	    return KINERT;
	}
    default:
	klispE_throw(K, "+: unsopported type");
	return KINERT;
    }
}

/* May throw an error */
/* GC: assumes n1 & n2 rooted */
TValue knum_times(klisp_State *K, TValue n1, TValue n2)
{
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int64_t res = (int64_t) ivalue(n1) * (int64_t) ivalue(n2);
	if (res >= (int64_t) INT32_MIN &&
	    res <= (int64_t) INT32_MAX) {
	    return i2tv((int32_t) res);
	} /* else fall through */
    }
    case K_TBIGINT: {
	kensure_bigint(n1);
	kensure_bigint(n2);
	return kbigint_times(K, n1, n2);
    }
    case K_TEINF:
	if (!ttiseinf(n1) || !ttiseinf(n2)) {
	    if (kfast_zerop(n1) || kfast_zerop(n2)) {
		/* report: #e+infinity * 0 has no primary value */
		klispE_throw(K, "*: result has no primary value");
		return KINERT;
	    } else
		return knum_same_signp(n1, n2)? KEPINF : KEMINF;
	} else
	    return (tv_equal(n1, n2))? KEPINF : KEMINF;
    default:
	klispE_throw(K, "*: unsopported type");
	return KINERT;
    }
}

/* May throw an error */
/* GC: assumes n1 & n2 rooted */
TValue knum_minus(klisp_State *K, TValue n1, TValue n2)
{
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int64_t res = (int64_t) ivalue(n1) - (int64_t) ivalue(n2);
	if (res >= (int64_t) INT32_MIN &&
	    res <= (int64_t) INT32_MAX) {
	    return i2tv((int32_t) res);
	} /* else fall through */
    }
    case K_TBIGINT: {
	kensure_bigint(n1);
	kensure_bigint(n2);
	return kbigint_minus(K, n1, n2);
    }
    case K_TEINF:
	if (!ttiseinf(n1))
	    return kneg_inf(n2);
	else if (!ttiseinf(n2))
	    return n1;
	if (tv_equal(n1, n2)) {
	    klispE_throw(K, "-: no primary value");
	    return KINERT;
	} else
	    return n1;
    default:
	klispE_throw(K, "-: unsopported type");
	return KINERT;
    }
}

/* GC: assumes n rooted */
TValue knum_abs(klisp_State *K, TValue n)
{
    switch(ttype(n)) {
    case K_TFIXINT: {
	int32_t i = ivalue(n);
	if (i != INT32_MIN)
	    return (i < 0? i2tv(-i) : n);
	/* if i == INT32_MIN, fall through */
	/* MAYBE: we could cache the bigint INT32_MAX+1 */
    }
    case K_TBIGINT: {
       /* this is needed for INT32_MIN, can't be in previous
	  case because it should be in the same block, remember
          the bigint is allocated on the stack. */
	kensure_bigint(n); 
	return kbigint_abs(K, n);
    }
    case K_TEINF:
	return KEPINF;
    default:
	/* shouldn't happen */
	klispE_throw(K, "abs: unsopported type");
	return KINERT;
    }
}

/* unlike the kernel gcd this returns |n| for gcd(n, 0) and gcd(0, n) and
 0 for gcd(0, 0) */
/* GC: assumes n1 & n2 rooted */
TValue knum_gcd(klisp_State *K, TValue n1, TValue n2)
{
    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int64_t gcd = kgcd32_64(ivalue(n1), ivalue(n2));
        /* May fail for gcd(INT32_MIN, INT32_MIN) because
	   it would return INT32_MAX+1 */
	if (kfit_int32_t(gcd)) 
	    return i2tv((int32_t) gcd);
	/* else fall through */
    }
    case K_TBIGINT: {
	kensure_bigint(n1);
	kensure_bigint(n2);
	return kbigint_gcd(K, n1, n2);
    }
    case K_TEINF:
	if (kfast_zerop(n2) || !ttiseinf(n1))
	    return knum_abs(K, n1);
	else if (kfast_zerop(n1) || !ttiseinf(n2))
	    return knum_abs(K, n2);
	else
	    return KEPINF;
    default:
	klispE_throw(K, "gcd: unsopported type");
	return KINERT;
    }
}

/* may throw an error if one of the arguments if zero */
/* GC: assumes n1 & n2 rooted */
TValue knum_lcm(klisp_State *K, TValue n1, TValue n2)
{
    /* get this out of the way first */
    if (kfast_zerop(n1) || kfast_zerop(n2)) {
	klispE_throw(K, "lcm: no primary value");
	return KINERT;
    }

    switch(max_ttype(n1, n2)) {
    case K_TFIXINT: {
	int64_t lcm = klcm32_64(ivalue(n1), ivalue(n2));
        /* May fail for lcm(INT32_MIN, 1) because
	   it would return INT32_MAX+1 */
	if (kfit_int32_t(lcm)) 
	    return i2tv((int32_t) lcm);
	/* else fall through */
    }
    case K_TBIGINT: {
	kensure_bigint(n1);
	kensure_bigint(n2);
	return kbigint_lcm(K, n1, n2);
    }
    case K_TEINF:
	return KEPINF;
    default:
	klispE_throw(K, "lcm: unsopported type");
	return KINERT;
    }
}

/* 12.5.4 + */
void kplus(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);
    /* cycles are allowed, loop counting pairs */
    int32_t cpairs; 
    int32_t pairs = check_typed_list(K, "+", "number", knumberp, true,
				     ptree, &cpairs);
    int32_t apairs = pairs - cpairs;

    TValue res;

    /* first the acyclic part */
    TValue ares = i2tv(0);
    krooted_vars_push(K, &ares);
    TValue tail = ptree;

    while(apairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);

	/* may throw an exception */
	ares = knum_plus(K, ares, first);
    }

    /* next the cyclic part */
    TValue cres = i2tv(0); /* push it only if needed */

    if (cpairs == 0) {
	/* speed things up if there is no cycle */
	res = ares;
	krooted_vars_pop(K);
    } else {
	bool all_zero = true;

	krooted_vars_push(K, &cres);
	while(cpairs--) {
	    TValue first = kcar(tail);
	    tail = kcdr(tail);

	    all_zero = all_zero && kfast_zerop(first);

	    cres = knum_plus(K, cres, first);
	}

	if (kfast_zerop(cres)) {
	    if (!all_zero) {
		/* report */
		klispE_throw(K, "+: result has no primary value");
		return;
	    }
	} else
	    cres = knegativep(cres)? KEMINF : KEPINF;
	res = knum_plus(K, ares, cres);
	krooted_vars_pop(K);
	krooted_vars_pop(K);
    }
    kapply_cc(K, res);
}

/* 12.5.5 * */
void ktimes(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);
    /* cycles are allowed, loop counting pairs */
    int32_t cpairs; 
    int32_t pairs = check_typed_list(K, "*", "number", knumberp, true,
				     ptree, &cpairs);
    int32_t apairs = pairs - cpairs;

    TValue res;

    /* first the acyclic part */
    TValue ares = i2tv(1);
    TValue tail = ptree;

    krooted_vars_push(K, &ares);
    while(apairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	ares = knum_times(K, ares, first);
    }

    /* next the cyclic part */
    TValue cres = i2tv(1);

    if (cpairs == 0) {
	/* speed things up if there is no cycle */
	res = ares;
	krooted_vars_pop(K);
    } else {
	bool all_one = true;

	krooted_vars_push(K, &cres);
	while(cpairs--) {
	    TValue first = kcar(tail);
	    tail = kcdr(tail);
	    all_one = all_one && kfast_onep(first);
	    cres = knum_times(K, cres, first);
	}

	/* think of cres as the product of an infinite series */
	if (kfast_zerop(cres)) 
	    ; /* do nothing */
	else if (kpositivep(cres) && knum_ltp(cres, i2tv(1)))
	    cres = i2tv(0);
	else if (kfast_onep(cres)) {
	    if (all_one)
		cres = i2tv(1);
	    else {
		klispE_throw(K, "*: result has no primary value");
		return;
	    }
	} else if (knum_gtp(cres, i2tv(1))) {
	    /* ASK JOHN: this is as per the report, but maybe we should check
	       that all elements are positive... */
	    cres = KEPINF;
	} else {
	    /* cycle result less than zero */
	    klispE_throw(K, "*: result has no primary value");
	    return;
	}

	res = knum_times(K, ares, cres);
	krooted_vars_pop(K);
	krooted_vars_pop(K);
    } 
    kapply_cc(K, res);
}

/* 12.5.6 - */
void kminus(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);
    /* cycles are allowed, loop counting pairs */
    int32_t cpairs;
    
    /* - in kernel (and unlike in scheme) requires at least 2 arguments */
    if (!ttispair(ptree) || !ttispair(kcdr(ptree))) {
	klispE_throw(K, "-: at least two values are required");
	return;
    } else if (!knumberp(kcar(ptree))) {
	klispE_throw(K, "-: bad type on first argument (expected number)");
	return;
    }
    TValue first_val = kcar(ptree);
    int32_t pairs = check_typed_list(K, "-", "number", knumberp, true,
				     kcdr(ptree), &cpairs);
    int32_t apairs = pairs - cpairs;

    TValue res;

    /* first the acyclic part */
    TValue ares = i2tv(0);
    TValue tail = kcdr(ptree);

    krooted_vars_push(K, &ares);

    while(apairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	ares = knum_plus(K, ares, first);
    }

    /* next the cyclic part */
    TValue cres = i2tv(0);

    if (cpairs == 0) {
	/* speed things up if there is no cycle */
	res = ares;
	krooted_vars_pop(K);
    } else {
	bool all_zero = true;

	krooted_vars_push(K, &cres);
	while(cpairs--) {
	    TValue first = kcar(tail);
	    tail = kcdr(tail);
	    all_zero = all_zero && kfast_zerop(first);
	    cres = knum_plus(K, cres, first);
	}

	if (kfast_zerop(cres)) {
	    if (!all_zero) {
		/* report */
		klispE_throw(K, "-: result has no primary value");
		return;
	    } 
	} else
	    cres = knegativep(cres)? KEMINF : KEPINF;
	res = knum_plus(K, ares, cres);
	krooted_vars_pop(K);
	krooted_vars_pop(K);
    } 
    /* now substract the sum of all the elements in the list to the first 
       value */
    krooted_tvs_push(K, res);
    res = knum_minus(K, first_val, res);
    krooted_tvs_pop(K);

    kapply_cc(K, res);
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

int32_t kfixint_div_mod(int32_t n, int32_t d, int32_t *res_mod) 
{
    int32_t div = n / d;
    int32_t mod = n % d;

    /* div, mod or div-and-mod */
    /* 0 <= mod0 < |d| */
    if (mod < 0) {
	if (d < 0) {
	    mod -= d;
	    ++div;
	} else {
	    mod += d;
	    --div;
	}
    }
    *res_mod = mod;
    return div;
}

int32_t kfixint_div0_mod0(int32_t n, int32_t d, int32_t *res_mod) 
{
    int32_t div = n / d;
    int32_t mod = n % d;

    /* div0, mod0 or div-and-mod0 */
    /*
    ** Adjust q and r so that:
    ** -|d/2| <= mod0 < |d/2| which is the same as
    ** dmin <= mod0 < dmax, where 
    ** dmin = -floor(|d/2|) and dmax = ceil(|d/2|) 
    */
    int32_t dmin = -((d<0? -d : d) / 2);
    int32_t dmax = ((d<0? -d : d) + 1) / 2;
	
    if (mod < dmin) {
	if (d < 0) {
	    mod -= d;
	    ++div;
	} else {
	    mod += d;
	    --div;
	}
    } else if (mod >= dmax) {
	if (d < 0) {
	    mod += d;
	    --div;
	} else {
	    mod -= d;
	    ++div;
	}
    }
    *res_mod = mod;
    return div;
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

    TValue tv_div, tv_mod;

    if (kfast_zerop(tv_d)) {
	klispE_throw_extra(K, name, ": division by zero");
	return;
    } 

    switch(max_ttype(tv_n, tv_d)) {
    case K_TFIXINT:
	/* NOTE: the only case were the result wouldn't fit in a fixint
	   is INT32_MIN divided by -1, resulting in INT32_MAX + 1.
	   The remainder is always < |tv_d| so no problem there, and
	   the quotient is always <= |tv_n|. All that said, the code to
	   correct the result returned by c operators / and % could cause
	   problems if d = INT32_MIN or d = INT32_MAX so just to be safe
	   we restrict d to be |d| < INT32_MAX and n to be 
	   |n| < INT32_MAX */
	if (!(ivalue(tv_n) <= INT32_MIN+2 || ivalue(tv_n) >= INT32_MAX-1 ||
	      ivalue(tv_d) <= INT32_MIN+2 || ivalue(tv_d) >= INT32_MAX-1)) {
	    int32_t div, mod;
	    if ((flags & FDIV_ZERO) == 0)
		div = kfixint_div_mod(ivalue(tv_n), ivalue(tv_d), &mod);
	    else
		div = kfixint_div0_mod0(ivalue(tv_n), ivalue(tv_d), &mod);
	    tv_div = i2tv(div);
	    tv_mod = i2tv(mod);
	    break;
	} /* else fall through */
    case K_TBIGINT:
	kensure_bigint(tv_n);
	kensure_bigint(tv_d);
	if ((flags & FDIV_ZERO) == 0)
	    tv_div = kbigint_div_mod(K, tv_n, tv_d, &tv_mod);
	else
	    tv_div = kbigint_div0_mod0(K, tv_n, tv_d, &tv_mod);
	break;
    case K_TEINF:
	if (ttiseinf(tv_n)) {
	    klispE_throw_extra(K, name, ": non finite dividend");
	    return;
	} else { /* if (ttiseinf(tv_d)) */
	    /* The semantics here are unclear, following the general
	       guideline of the report that says that if an infinity is 
	       involved it should be understand as a limit. In that
	       case once the divisor is greater in magnitude than the
	       dividend the division stabilizes itself at q = 0; r = n
	       if both have the same sign, and q = 1; r = +infinity if
	       both have different sign (but in that case !(r < |d|)
	       !!) */ 
            /* RATIONALE: if q were 0 we can't accomplish 
	       q * d + r = n because q * d is undefined, if q isn't zero
	       then, either q*d + r is infinite or undefined so
	       there's no good q.  on the other hand if we want 
	       n - q*d = r & 0 <= r < d, r can't be infinite because it
	       would be equal to d, but q*d is infinite, so there's no
	       way out */
	    /* throw an exception, until this is resolved */
	    /* ASK John */
	    klispE_throw_extra(K, name, ": non finite divisor");
	    return;
	}
    default:
	klispE_throw_extra(K, name, ": unsopported type");
	return;
    }

    TValue res;
    if (flags & FDIV_DIV) {
	if (flags & FDIV_MOD) { /* return both div and mod */
	    krooted_tvs_push(K, tv_div);
	    krooted_tvs_push(K, tv_mod);
	    res = klist(K, 2, tv_div, tv_mod);
	    krooted_tvs_pop(K);
	    krooted_tvs_pop(K);
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
bool kpositivep(TValue n) 
{ 
    switch (ttype(n)) {
    case K_TFIXINT:
    case K_TEINF:
	return ivalue(n) > 0;
    case K_TBIGINT:
	return kbigint_positivep(n);
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool knegativep(TValue n) 
{ 
    switch (ttype(n)) {
    case K_TFIXINT:
    case K_TEINF:
	return ivalue(n) < 0;
    case K_TBIGINT:
	return kbigint_negativep(n);
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

/* n is finite */
bool koddp(TValue n) 
{ 
    switch (ttype(n)) {
    case K_TFIXINT:
	return (ivalue(n) & 1) != 0; 
    case K_TBIGINT:
	return kbigint_oddp(n);
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

bool kevenp(TValue n) 
{ 
    switch (ttype(n)) {
    case K_TFIXINT:
	return (ivalue(n) & 1) == 0; 
    case K_TBIGINT:
	return kbigint_evenp(n);
    default:
	/* shouldn't happen */
	assert(0);
	return false;
    }
}

/* 12.5.12 abs */
void kabs(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, "abs", ptree, "number", knumberp, n);

    TValue res = knum_abs(K, n);
    kapply_cc(K, res);
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
    int32_t dummy; /* don't care about count of cycle pairs */
    int32_t pairs = check_typed_list(K, name, "number", knumberp, true, ptree,
	&dummy);
    
    TValue res;

    if (minp) {
	res = KEPINF;
    } else {
	res = KEMINF;
    }

    TValue tail = ptree;
    bool (*cmp)(TValue, TValue) = minp? knum_ltp : knum_gtp;

    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);

	if ((*cmp)(first, res))
	    res = first;
    }
    kapply_cc(K, res);
}

/* 12.5.14 gcm, lcm */
void kgcd(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    /* cycles are allowed, loop counting pairs */
    int32_t dummy; /* don't care about count of cycle pairs */
    int32_t pairs = check_typed_list(K, "gcd", "number", kimp_intp, true,
				     ptree, &dummy);

    TValue res = i2tv(0);
    krooted_vars_push(K, &res);

    if (pairs == 0) {
	res = KEPINF; /* report: (gcd) = #e+infinity */
    } else {
	TValue tail = ptree;
	bool seen_finite_non_zero = false; 
	/* res = 0 */

	while(pairs--) {
	    TValue first = kcar(tail);
	    tail = kcdr(tail);
	    seen_finite_non_zero |= 
		(!ttiseinf(first) && !kfast_zerop(first));
	    res = knum_gcd(K, res, first);
	}

	if (!seen_finite_non_zero) {
           /* report */
	    klispE_throw(K, "gcd: result has no primary value");
	    return;
	}
    }

    krooted_vars_pop(K);
    kapply_cc(K, res);
}

void klcm(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    /* cycles are allowed, loop counting pairs */
    int32_t dummy; /* don't care about count of cycle pairs */
    int32_t pairs = check_typed_list(K, "lcm", "number", kimp_intp, true, 
				     ptree, &dummy);

    /* report: this will cover the case of (lcm) = 1 */
    TValue res = i2tv(1);
    krooted_vars_push(K, &res);
    
    TValue tail = ptree;
    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	/* This will check that neither is zero */
	res = knum_lcm(K, res, first);
    }

    krooted_vars_pop(K);
    kapply_cc(K, res);
}

