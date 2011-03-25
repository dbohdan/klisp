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

/* 5.10.1 $let */
/* TODO */

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
