/*
** kgkd_vars.c
** Keyed Dynamic Variables features for the ground environment
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
#include "kcontinuation.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kenvironment.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgcontinuations.h" /* for do_pass_value / guards */
#include "kgkd_vars.h"

/*
** A dynamic key is a pair with a boolean in the car indicating if the
** variable is bound and an arbitrary object in the cdr representing the
** currently bound value.
*/

/* Helpers for make-keyed-dynamic-variable */

/* accesor returned */
void do_access(klisp_State *K, TValue *xparams, TValue ptree, 
	       TValue denv)
{
    /*
    ** xparams[0]: dynamic key 
    */
    check_0p(K, "keyed-dynamic-get", ptree);
    UNUSED(denv);
    TValue key = xparams[0];

    if (kis_true(kcar(key))) {
	kapply_cc(K, kcdr(key));
    } else {
	klispE_throw(K, "keyed-dynamic-get: variable is unbound");
	return;
    }
}

/* continuation to set the key to the old value on normal return */
void do_unbind(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: dynamic key
    ** xparams[1]: old flag
    ** xparams[2]: old value
    */

    TValue key = xparams[0];
    TValue old_flag = xparams[1];
    TValue old_value = xparams[2];

    kset_car(key, old_flag);
    kset_cdr(key, old_value);
    /* pass along the value returned to this continuation */
    kapply_cc(K, obj);
}

/* operative for setting the key to the new/old flag/value */
void do_set_pass(klisp_State *K, TValue *xparams, TValue ptree,
    TValue denv)
{
    /*
    ** xparams[0]: dynamic key
    ** xparams[1]: flag
    ** xparams[2]: value
    */
    TValue key = xparams[0];
    TValue flag = xparams[1];
    TValue value = xparams[2];
    UNUSED(denv);

    kset_car(key, flag);
    kset_cdr(key, value);

    /* pass to next interceptor/ final destination */
    /* ptree is as for interceptors: (obj divert) */
    TValue obj = kcar(ptree);
    kapply_cc(K, obj);
}

/* create continuation to set the key on both normal return and
   abnormal passes */
/* TODO: reuse the code for guards in kgcontinuations.c */

/* GC: this assumes that key is rooted */
inline TValue make_bind_continuation(klisp_State *K, TValue key,
				     TValue old_flag, TValue old_value, 
				     TValue new_flag, TValue new_value)
{
    TValue unbind_cont = kmake_continuation(K, kget_cc(K), 
					    do_unbind, 3, key, old_flag, 
					    old_value);
    krooted_tvs_push(K, unbind_cont);
    /* create the guards to guarantee that the values remain consistent on
       abnormal passes (in both directions) */
    TValue exit_int = kmake_operative(K, do_set_pass, 
				      3, key, old_flag, old_value);
    krooted_tvs_push(K, exit_int);
    TValue exit_guard = kcons(K, K->root_cont, exit_int);
    krooted_tvs_pop(K); /* already rooted in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* already rooted in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_int = kmake_operative(K, do_set_pass, 
				       3, key, new_flag, new_value);
    krooted_tvs_push(K, entry_int);
    TValue entry_guard = kcons(K, K->root_cont, entry_int);
    krooted_tvs_pop(K); /* already rooted in guard */
    krooted_tvs_push(K, entry_guard);
    TValue entry_guards = kcons(K, entry_guard, KNIL);
    krooted_tvs_pop(K); /* already rooted in guards */
    krooted_tvs_push(K, entry_guards);


    /* NOTE: in the stack now we have the unbind cont & two guard lists */
    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, unbind_cont, 
					   do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
					   do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);

    /* unbind_cont & 2 guard_lists */
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);
    /* env & outer_cont */
    krooted_tvs_pop(K); krooted_tvs_pop(K);
    
    return inner_cont;
}

/* binder returned */
void do_bind(klisp_State *K, TValue *xparams, TValue ptree, 
	       TValue denv)
{
    /*
    ** xparams[0]: dynamic key 
    */
    bind_2tp(K, "keyed-dynamic-bind", ptree, "any", anytype, obj,
	      "combiner", ttiscombiner, comb);
    UNUSED(denv); /* the combiner is called in an empty environment */
    TValue key = xparams[0];
    /* GC: root intermediate objs */
    TValue new_flag = KTRUE;
    TValue new_value = obj;
    TValue old_flag = kcar(key);
    TValue old_value = kcdr(key);
    /* set the var to the new object */
    kset_car(key, new_flag);
    kset_cdr(key, new_value);
    /* create a continuation to set the var to the correct value/flag on both
     normal return and abnormal passes */
    TValue new_cont = make_bind_continuation(K, key, old_flag, old_value,
					     new_flag, new_value);
    kset_cc(K, new_cont); /* implicit rooting */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue expr = kcons(K, comb, KNIL);
    krooted_tvs_pop(K);
    ktail_eval(K, expr, env)
}

/* 10.1.1 make-keyed-dynamic-variable */
void make_keyed_dynamic_variable(klisp_State *K, TValue *xparams, 
				 TValue ptree, TValue denv)
{
    UNUSED(denv); 
    UNUSED(xparams);

    check_0p(K, "make-keyed-dynamic-variable", ptree);
    TValue key = kcons(K, KFALSE, KINERT);
    krooted_tvs_push(K, key);
    TValue a = kmake_applicative(K, do_access, 1, key);
    krooted_tvs_push(K, a);
    TValue b = kmake_applicative(K, do_bind, 1, key);
    krooted_tvs_push(K, b);
    TValue ls = klist(K, 2, b, a);

    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    kapply_cc(K, ls);
}

