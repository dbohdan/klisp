/*
** kgcombiners.c
** Combiners features for the ground environment
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
#include "koperative.h"
#include "kapplicative.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgpair_mut.h" /* for copy_es_immutable_h */
#include "kgenv_mut.h" /* for match */
#include "kgcontrol.h" /* for do_seq */
#include "kgcombiners.h"

/* Helper (used by $vau & $lambda) */
void do_vau(klisp_State *K, TValue *xparams, TValue obj, TValue denv);

/* 4.10.1 operative? */
/* uses typep */

/* 4.10.2 applicative? */
/* uses typep */

/* 4.10.3 $vau */
/* 5.3.1 $vau */
void Svau(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    bind_al2p(K, "$vau", ptree, vptree, vpenv, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, "$vau", vptree, vpenv);
    /* the body should be a list */
    (void)check_list(K, "$vau", vbody);
    vbody = copy_es_immutable_h(K, "$vau", vbody);

    TValue new_op = make_operative(K, do_vau, 4, vptree, vpenv, vbody, denv);
    kapply_cc(K, new_op);
}

void do_vau(klisp_State *K, TValue *xparams, TValue obj, TValue denv)
{
    /*
    ** xparams[0]: ptree
    ** xparams[1]: penv
    ** xparams[2]: body
    ** xparams[3]: senv
    */
    TValue ptree = xparams[0];
    TValue penv = xparams[1];
    TValue body = xparams[2];
    TValue senv = xparams[3];

    /* bindings in an operative are in a child of the static env */
    TValue env = kmake_environment(K, senv);
    /* TODO use name from operative */
    match(K, "[user-operative]", env, ptree, obj);
    kadd_binding(K, env, penv, denv);
    
    if (ttisnil(body)) {
	kapply_cc(K, KINERT);
    } else {
	/* this is needed because seq continuation doesn't check for 
	   nil sequence */
	TValue tail = kcdr(body);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, env);
	    kset_cc(K, new_cont);
	} 
	ktail_eval(K, kcar(body), env);
    }
}

/* 4.10.4 wrap */
void wrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1tp(K, "wrap", ptree, "combiner", ttiscombiner, comb);
    TValue new_app = kwrap(K, comb);
    kapply_cc(K, new_app);
}

/* 4.10.5 unwrap */
void unwrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1tp(K, "unwrap", ptree, "applicative", ttisapplicative, app);
    TValue underlying = kunwrap(K, app);
    kapply_cc(K, underlying);
}

/* 5.3.1 $vau */
/* DONE: above, together with 4.10.4 */

/* 5.3.2 $lambda */
void Slambda(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    bind_al2p(K, "$lambda", ptree, vptree, vpenv, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, "$lambda", vptree, vpenv);
    /* the body should be a list */
    (void)check_list(K, "$lambda", vbody);
    vbody = copy_es_immutable_h(K, "$lambda", vbody);

    TValue new_app = make_applicative(K, do_vau, 4, vptree, vpenv, vbody, denv);
    kapply_cc(K, new_app);
}

/* 5.5.1 apply */
void apply(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_al2p(K, "apply", ptree, app, obj, maybe_env);

    if(!ttisapplicative(app)) {
	klispE_throw(K, "apply: Bad type on first argument "
		     "(expected applicative)");    
	return;
    }
    TValue env;
    /* TODO move to an inlinable function */
    if (ttisnil(maybe_env)) {
	env = kmake_empty_environment(K);
    } else if (ttispair(maybe_env) && ttisnil(kcdr(maybe_env))) {
	env = kcar(maybe_env);
	if (!ttisenvironment(env)) {
	    klispE_throw(K, "apply: Bad type on optional argument "
			 "(expected environment)");    
	}
    } else {
	klispE_throw(K, "apply: Bad ptree structure (in optional argument)");
    }
    TValue expr = kcons(K, kunwrap(K, app), obj);
    ktail_eval(K, expr, env);
}

/* 5.9.1 map */
/* TODO */
