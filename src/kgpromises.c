/*
** kgencapsulations.c
** Encapsulations features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpromise.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgpromises.h"

/* SOURCE_NOTE: this is mostly an adaptation of the library derivation
   in the report */

/* 9.1.1 promise? */
/* uses typep */

/* Helper for force */
void handle_result(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: promise
    */
    TValue prom = xparams[0];

    /* check to see if promise was determined before the eval completed */
    if (ttisnil(kpromise_maybe_env(prom))) {
	/* discard obj, return previous result */
	kapply_cc(K, kpromise_exp(prom));
    } else if (ttispromise(obj)) {
	/* force iteratively, by sharing pairs so that when obj
	 determines a value, prom also does */
	TValue node = kpromise_node(obj);
	kpromise_node(prom) = node;
	TValue expr = kpromise_exp(prom);
	TValue maybe_env = kpromise_maybe_env(prom);
	if (ttisnil(maybe_env)) {
	    /* promise was already determined */
	    kapply_cc(K, expr);
	} else {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
						 handle_result, 1, prom);
	    kset_cc(K, new_cont);
	    ktail_eval(K, expr, maybe_env);
	}
    } else {
	/* memoize result */
	TValue node = kpromise_node(prom);
	kset_car(node, obj);
	kset_cdr(node, KNIL);
    }
}

/* 9.1.2 force */
void force(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1p(K, "force", ptree, obj);
    if (!ttispromise(obj)) {
	/* non promises force to themselves */
	kapply_cc(K, obj);
    } else if (ttisnil(kpromise_maybe_env(obj))) {
	/* promise was already determined */
	kapply_cc(K, kpromise_exp(obj));
    } else {
	TValue expr = kpromise_exp(obj);
	TValue env = kpromise_maybe_env(obj);
	TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     handle_result, 1, obj);
	kset_cc(K, new_cont);
	ktail_eval(K, expr, env);
    }
}

/* 9.1.3 $lazy */
void Slazy(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);

    bind_1p(K, "$lazy", ptree, exp);
    TValue new_prom = kmake_promise(K, KNIL, KNIL, exp, denv);
    kapply_cc(K, new_prom);
}

/* 9.1.4 memoize */
void memoize(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1p(K, "memoize", ptree, exp);
    TValue new_prom = kmake_promise(K, KNIL, KNIL, exp, KNIL);
    kapply_cc(K, new_prom);
}