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
    int32_t cpairs;
    int32_t pairs = check_list(K, name, true, ptree, &cpairs);

    TValue tail = ptree;
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
    int32_t cpairs;
    int32_t pairs = check_list(K, name, true, ptree, &cpairs);

    /* cyclical list require an extra comparison of the last
       & first element of the cycle */
    int32_t comps = cpairs? pairs : pairs - 1;

    TValue tail = ptree;
    bool res = true;

    /* check the type while checking the predicate.
       Keep going even if the result is false to catch errors in 
       type */

    if (comps == 0) {
	/* this case has to be here because otherwise there is no check
	   for the type of the lone operand */
	TValue first = kcar(tail);
	if (!(*typep)(first)) {
	    /* TODO show expected type */
	    klispE_throw_extra(K, name, ": bad argument type");
	    return;
	}
    }

    while(comps-- > 0) { /* comps could be -1 if ptree is () */
	TValue first = kcar(tail);
	tail = kcdr(tail); /* tail only advances one place per iteration */
	TValue second = kcar(tail);

	if (!(*typep)(first) || !(*typep)(second)) {
	    /* TODO show expected type */
	    klispE_throw_extra(K, name, ": bad argument type");
	    return;
	}
	res &= (*predp)(first, second);
    }
    kapply_cc(K, b2tv(res));
}

/* typed finite list. Structure error should be throw before type errors */
int32_t check_typed_list(klisp_State *K, char *name, char *typename,
			 bool (*typep)(TValue), bool allow_infp, TValue obj,
			 int32_t *cpairs)
{
    TValue tail = obj;
    int32_t pairs = 0;
    bool type_errorp = false;

    while(ttispair(tail) && !kis_marked(tail)) {
	/* even if there is a type error continue checking the structure */
	type_errorp |= !(*typep)(kcar(tail));
	kset_mark(tail, i2tv(pairs));
	tail = kcdr(tail);
	++pairs;
    }
    *cpairs = ttispair(tail)? (pairs - ivalue(kget_mark(tail))) : 0;
    unmark_list(K, obj);

    if (!ttispair(tail) && !ttisnil(tail)) {
	klispE_throw_extra(K, name , ": expected finite list"); 
	return 0;
    } else if(ttispair(tail) & !allow_infp) {
	klispE_throw_extra(K, name , ": expected finite list"); 
	return 0;
    } else if (type_errorp) {
	/* TODO put type name too */
	klispE_throw_extra(K, name , ": bad operand type"); 
	return 0;
    }
    return pairs;
}

int32_t check_list(klisp_State *K, char *name, bool allow_infp,
			  TValue obj, int32_t *cpairs)
{
    TValue tail = obj;
    int pairs = 0;
    while(ttispair(tail) && !kis_marked(tail)) {
	kset_mark(tail, i2tv(pairs));
	tail = kcdr(tail);
	++pairs;
    }
    *cpairs = ttispair(tail)? (pairs - ivalue(kget_mark(tail))) : 0;
    unmark_list(K, obj);

    if (!ttispair(tail) && !ttisnil(tail)) {
	klispE_throw_extra(K, name , ": expected finite list"); 
	return 0;
    } else if(ttispair(tail) & !allow_infp) {
	klispE_throw_extra(K, name , ": expected finite list"); 
	return 0;
    } else {
	return pairs;
    }
}
