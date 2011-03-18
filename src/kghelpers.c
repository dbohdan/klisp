/*
** kghelpers.c
** Helper macros and functions for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "kghelpers.h"
#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kerror.h"
#include "ksymbol.h"

void typep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type tag (as by i2tv)
    */
    int32_t tag = ivalue(xparams[1]);

    /* check the ptree is a list while checking the predicate.
       Keep going even if the result is false to catch errors in 
       ptree structure */
    bool res = true;

    TValue tail = ptree;
    while(ttispair(tail) && kis_unmarked(tail)) {
	kmark(tail);
	res &= ttype(kcar(tail)) == tag;
	tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttispair(tail) || ttisnil(tail)) {
	kapply_cc(K, b2tv(res));
    } else {
	char *name = ksymbol_buf(xparams[0]);
	klispE_throw_extra(K, name, ": expected list");
	return;
    }
}

void ftypep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: fn pointer (as a void * in a user TValue)
    */
    bool (*fn)(TValue obj) = pvalue(xparams[1]);

    /* check the ptree is a list while checking the predicate.
       Keep going even if the result is false to catch errors in 
       ptree structure */
    bool res = true;

    TValue tail = ptree;
    while(ttispair(tail) && kis_unmarked(tail)) {
	kmark(tail);
	res &= (*fn)(kcar(tail));
	tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttispair(tail) || ttisnil(tail)) {
	kapply_cc(K, b2tv(res));
    } else {
	char *name = ksymbol_buf(xparams[0]);
	klispE_throw_extra(K, name, ": expected list");
	return;
    }
}

void ftyped_predp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type fn pointer (as a void * in a user TValue)
    ** xparams[2]: fn pointer (as a void * in a user TValue)
    */
    char *name = ksymbol_buf(xparams[0]);
    bool (*typep)(TValue obj) = pvalue(xparams[1]);
    bool (*predp)(TValue obj) = pvalue(xparams[2]);

    /* check the ptree is a list first to allow the structure
       errors to take precedence over the type errors. */

    TValue tail = ptree;
    int32_t pairs = 0;
    while(ttispair(tail) && kis_unmarked(tail)) {
	pairs++;
	kmark(tail);
	tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (!ttispair(tail) && !ttisnil(tail)) {
	klispE_throw_extra(K, name, ": expected list");
	return;
    }

    tail = ptree;
    bool res = true;

    /* check the type while checking the predicate.
       Keep going even if the result is false to catch errors in 
       type */
    while(pairs--) {
	TValue first = kcar(tail);

	if (!(*typep)(first)) {
	    /* TODO show expected type */
	    klispE_throw_extra(K, name, ": bad argument type");
	    return;
	}
	res &= (*predp)(first);
	tail = kcdr(tail);
    }
    kapply_cc(K, b2tv(res));
}

void ftyped_bpredp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type fn pointer (as a void * in a user TValue)
    ** xparams[2]: fn pointer (as a void * in a user TValue)
    */
    char *name = ksymbol_buf(xparams[0]);
    bool (*typep)(TValue obj) = pvalue(xparams[1]);
    bool (*predp)(TValue obj1, TValue obj2) = pvalue(xparams[2]);

    /* check the ptree is a list first to allow the structure
       errors to take precedence over the type errors. */
    TValue tail = ptree;
    int32_t pairs = 0;

    while(ttispair(tail) && kis_unmarked(tail)) {
	pairs++;
	kmark(tail);
	tail = kcdr(tail);
    }
    unmark_list(K, ptree);
    int32_t comps;
    if (ttisnil(tail)) {
	comps = pairs - 1;
    } else if (ttispair(tail)) {
	/* cyclical list require an extra comparison of the last
	  & first element of the cycle */
	comps = pairs;
    } else {
	klispE_throw_extra(K, name, ": expected list");
	return;
    }

    tail = ptree;
    bool res = true;

    /* check the type while checking the predicate.
       Keep going even if the result is false to catch errors in 
       type */
    /* it checks > 0 because if ptree is nil comps = -1 */
    while(comps-- > 0) {
	TValue first = kcar(tail);
	tail = kcdr(tail); /* tail only advances one place per iteration */
	TValue second = kcar(tail);

	if (!(*typep)(first) && !(*typep)(second)) {
	    /* TODO show expected type */
	    klispE_throw_extra(K, name, ": bad argument type");
	    return;
	}
	res &= (*predp)(first, second);
    }
    kapply_cc(K, b2tv(res));
}
