/*
** kgenvironments.c
** Environments features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kenvironment.h"
#include "kcontinuation.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgenvironments.h"
#include "kgenv_mut.h" /* for check_ptree */ 
#include "kgpair_mut.h" /* for copy_es_immutable_h */
/* MAYBE: move the above to kghelpers.h */

/* 4.8.1 environment? */
/* uses typep */

/* 4.8.2 ignore? */
/* uses typep */

/* 4.8.3 eval */
void eval(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    bind_2tp(K, "eval", ptree, "any", anytype, expr,
	     "environment", ttisenvironment, env);

    ktail_eval(K, expr, env);
}

/* 4.8.4 make-environment */
void make_environment(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    TValue new_env;
    if (ttisnil(ptree)) {
	new_env = kmake_empty_environment(K);
	kapply_cc(K, new_env);
    } else if (ttispair(ptree) && ttisnil(kcdr(ptree))) {
	/* special common case of one parent, don't keep a list */
	TValue parent = kcar(ptree);
	if (ttisenvironment(parent)) {
	    new_env = kmake_environment(K, parent);
	    kapply_cc(K, new_env);
	} else {
	    klispE_throw(K, "make-environment: not an environment in "
			 "parent list");
	    return;
	}
    } else {
	/* this is the general case, copy the list but without the
	   cycle if there is any */
	TValue parents = check_copy_env_list(K, "make-environment", ptree);
	new_env = kmake_environment(K, parents);
	kapply_cc(K, new_env);
    }
}

/* Helpers for all the let family */

/* 
** The split-let-bindings function has two cases:
** the 'lets' with a star ($let* and $letrec) allow repeated symbols
** in different bidings (each binding is a different ptree whereas
** in $let, $letrec, $let-redirect and $let-safe, all the bindings
** are collected in a single ptree).
** In both cases the value returned is a list of cars of bindings and
** exprs is modified to point to a list of cadrs of bindings.
** The ptrees are copied as by copy-es-immutable (as with $vau & $lambda)
** If bindings is not finite (or not a list) an error is signaled.
*/

TValue split_check_let_bindings(klisp_State *K, char *name, TValue bindings, 
				TValue *exprs, bool starp)
{
    TValue dummy_cars = kcons(K, KNIL, KNIL);
    TValue last_car_pair = dummy_cars;
    TValue dummy_cadrs = kcons(K, KNIL, KNIL);
    TValue last_cadr_pair = dummy_cadrs;

    TValue tail = bindings;

    while(ttispair(tail) && !kis_marked(tail)) {
	kmark(tail);
	TValue first = kcar(tail);
	if (!ttispair(first) || !ttispair(kcdr(first)) ||
	        !ttisnil(kcddr(first))) {
	    unmark_list(K, bindings);
	    klispE_throw_extra(K, name, ": bad structure in bindings");
	    return KNIL;
	}
	
	TValue new_car = kcons(K, kcar(first), KNIL);
	kset_cdr(last_car_pair, new_car);
	last_car_pair = new_car;
	TValue new_cadr = kcons(K, kcadr(first), KNIL);
	kset_cdr(last_cadr_pair, new_cadr);
	last_cadr_pair = new_cadr;

	tail = kcdr(tail);
    }

    unmark_list(K, bindings);

    if (!ttispair(tail) && !ttisnil(tail)) {
	klispE_throw_extra(K, name, ": expected list");
	return KNIL;
    } else if(ttispair(tail)) {
	klispE_throw_extra(K, name , ": expected finite list"); 
	return KNIL;
    } else {
	*exprs = kcdr(dummy_cadrs);
	TValue res;
	if (starp) {
	    /* all bindings are consider individual ptrees in these 'let's,
	       replace each ptree with its copy (after checking of course) */
	    tail = kcdr(dummy_cars);
	    while(!ttisnil(tail)) {
		TValue first = kcar(tail);
		TValue copy = check_copy_ptree(K, name, first, KIGNORE);
		kset_car(tail, copy);
		tail = kcdr(tail);
	    }
	    res = kcdr(dummy_cars);
	} else {
	    /* all bindings are consider one ptree in these 'let's */
	    res = check_copy_ptree(K, name, kcdr(dummy_cars), KIGNORE);
	}
	return res;
    }
}

/* 5.10.1 $let */
/* TEMP: for now this only checks the parameters and makes copies */
/* XXX: it doesn't do any evaluation or env creation */
void Slet(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: symbol name
    */
    UNUSED(denv);
    char *name = ksymbol_buf(xparams[0]);
    bind_al1p(K, name, ptree, bindings, body);

    TValue exprs;
    TValue btree = split_check_let_bindings(K, name, bindings, &exprs, false);
    int32_t dummy;
    UNUSED(check_list(K, name, true, body, &dummy));
    body = copy_es_immutable_h(K, name, body, false);

    /* XXX */
    TValue res = kcons(K, btree, kcons(K, exprs, body));
    kapply_cc(K, res);
}

/* 6.7.1 $binds? */
/* TODO */

/* 6.7.2 get-current-environment */
void get_current_environment(klisp_State *K, TValue *xparams, TValue ptree, 
			     TValue denv)
{
    UNUSED(xparams);
    check_0p(K, "get-current-environment", ptree);
    kapply_cc(K, denv);
}

/* 6.7.3 make-kernel-standard-environment */
void make_kernel_standard_environment(klisp_State *K, TValue *xparams, 
				      TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    check_0p(K, "make-kernel-standard-environment", ptree);
    
    TValue new_env = kmake_environment(K, K->ground_env);
    kapply_cc(K, new_env);
}

/* 6.7.4 $let* */
/* TODO */

/* 6.7.5 $letrec */
/* TODO */

/* 6.7.6 $letrec* */
/* TODO */

/* 6.7.7 $let-redirect */
/* TODO */

/* 6.7.8 $let-safe */
/* TODO */

/* 6.7.9 $remote-eval */
void Sremote_eval(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_2p(K, "$remote-eval", ptree, obj, env_exp);

    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					 do_remote_eval, 1, obj);
    kset_cc(K, new_cont);

    ktail_eval(K, env_exp, denv);
}

/* Helper for $remote-eval */
void do_remote_eval(klisp_State *K, TValue *xparams, TValue obj)
{
    if (!ttisenvironment(obj)) {
	klispE_throw(K, "$remote-eval: bad type from second operand "
		     "evaluation (expected environment)");
	return;
    } else {
	TValue eval_exp = xparams[0];
	ktail_eval(K, eval_exp, obj);
    }
}

/* 6.7.10 $bindings->environment */
/* TODO */
