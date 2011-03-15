/*
** kgcontinuations.h
** Continuations features for the ground environment
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
#include "kapplicative.h"
#include "koperative.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgcontinuations.h"

/* 7.1.1 continuation? */
/* uses typep */

/* 7.2.2 call/cc */
void call_cc(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    bind_1tp(K, "call/cc", ptree, "combiner", ttiscombiner, comb);

    /* GC: root pairs */
    TValue expr = kcons(K, comb, kcons(K, kget_cc(K), KNIL));
    ktail_eval(K, expr, denv);
}

/* Helper for extend-continuation */
void do_extended_cont(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: applicative
    ** xparams[1]: environment
    */
    TValue app = xparams[0];
    TValue underlying = kunwrap(K, app);
    TValue env = xparams[1];

    TValue expr = kcons(K, underlying, obj);
    ktail_eval(K, expr, env);
}

/* 7.2.3 extend-continuation */
void extend_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_al2tp(K, "extend-continuation", ptree, 
	       "continuation", ttiscontinuation, cont, 
	       "applicative", ttisapplicative, app, 
	       maybe_env);

    TValue env = (get_opt_tpar(K, "apply", K_TENVIRONMENT, &maybe_env))?
	maybe_env : kmake_empty_environment(K);

    TValue new_cont = kmake_continuation(K, cont, KNIL, KNIL, 
					 do_extended_cont, 2, app, env);
    kapply_cc(K, new_cont);
}

/* 7.2.4 guard-continuation */
/* TODO */


/* helper for continuation->applicative */
void cont_app(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 7.2.5 continuation->applicative */
/* TODO: look out for guards and dynamic variables */
void continuation_applicative(klisp_State *K, TValue *xparams, TValue ptree, 
			      TValue denv)
{
    (void) xparams;
    bind_1tp(K, "continuation->applicative", ptree, "continuation",
	     ttiscontinuation, cont);

    TValue app = make_applicative(K, cont_app, 1, cont);
    kapply_cc(K, app);
}

/* this passes the operand tree to the continuation */
void cont_app(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    TValue cont = xparams[0];
    /* TODO: look out for guards and dynamic variables */
    /* should be probably handled in kcall_cont() */
    kcall_cont(K, cont, ptree);
}

/* 7.2.6 root-continuation */
/* done in kground.c/krepl.c */

/* 7.2.7 error-continuation */
/* done in kground.c/krepl.c */

/* 
** 7.3 Library features
*/

/* 7.3.1 apply-continuation */
void apply_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv)
{
    (void) xparams;
    (void) denv;

    bind_2tp(K, "apply-continuation", ptree, "continuation", ttiscontinuation,
	     cont, "any", anytype, obj);

    /* TODO: look out for guards and dynamic variables */
    /* should be probably handled in kcall_cont() */
    kcall_cont(K, cont, obj);
}

/* 7.3.2 $let/cc */
/* TODO */

/* 7.3.3 guard-dynamic-extent */
/* TODO */

/* 7.3.4 exit */    
/* TODO */
