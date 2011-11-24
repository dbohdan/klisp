/*
** kgcontinuations.c
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
#include "kgcontrol.h" /* for seq helpers in $let/cc */

/* 7.1.1 continuation? */
/* uses typep */

/* 7.2.2 call/cc */
void call_cc(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    bind_1tp(K, ptree, "combiner", ttiscombiner, comb);

    TValue expr = klist(K, 2, comb, kget_cc(K));
    ktail_eval(K, expr, denv);
}

/* Helper for extend-continuation */
void do_extended_cont(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: applicative
    ** xparams[1]: environment
    */
    TValue app = xparams[0];
    TValue underlying = kunwrap(app);
    TValue env = xparams[1];

    TValue expr = kcons(K, underlying, obj);
    ktail_eval(K, expr, env);
}

/* 7.2.3 extend-continuation */
void extend_continuation(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

    bind_al2tp(K, ptree, 
	       "continuation", ttiscontinuation, cont, 
	       "applicative", ttisapplicative, app, 
	       maybe_env);

    TValue env = (get_opt_tpar(K, maybe_env, "environment", ttisenvironment))?
	maybe_env : kmake_empty_environment(K);

    krooted_tvs_push(K, env);
    TValue new_cont = kmake_continuation(K, cont, 
					 do_extended_cont, 2, app, env);
    krooted_tvs_pop(K);
    kapply_cc(K, new_cont);
}

/* Helpers for guard-continuation (& guard-dynamic-extent) */

/* this is used for inner & outer continuations, it just
   passes the value. xparams is not actually empty, it contains
   the entry/exit guards, but they are used only in 
   continuation->applicative (that is during abnormal passes) */
void do_pass_value(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    UNUSED(xparams);
    kapply_cc(K, obj);
}

#define singly_wrapped(obj_) (ttisapplicative(obj_) && \
			      ttisoperative(kunwrap(obj_)))

/* this unmarks root before throwing any error */
/* TODO: this isn't very clean, refactor */

/* GC: assumes obj & root are rooted, dummy1 is in use */
inline TValue check_copy_single_entry(klisp_State *K, char *name,
				      TValue obj, TValue root)
{
    if (!ttispair(obj) || !ttispair(kcdr(obj)) || 
	    !ttisnil(kcddr(obj))) {
	unmark_list(K, root);
	klispE_throw_simple(K, "Bad entry (expected list of length 2)");
	return KINERT;
    } 
    TValue cont = kcar(obj);
    TValue app = kcadr(obj);

    if (!ttiscontinuation(cont)) {
	unmark_list(K, root);
	klispE_throw_simple(K, "Bad type on first element (expected " 
		     "continuation)");				     
	return KINERT;
    } else if (!singly_wrapped(app)) { 
	unmark_list(K, root);
	klispE_throw_simple(K, "Bad type on second element (expected " 
		     "singly wrapped applicative)");				     
	return KINERT; 
    }

    /* save the operative directly, don't waste space/time
     with a list, use just a pair */
    return kcons(K, cont, kunwrap(app)); 
}

/* the guards are probably generated on the spot so we don't check
   for immutability and copy it anyways */
/* GC: Assumes obj is rooted */
TValue check_copy_guards(klisp_State *K, char *name, TValue obj)
{
    if (ttisnil(obj)) {
	return obj;
    } else {
	TValue last_pair = kget_dummy1(K);
	TValue tail = obj;
    
	while(ttispair(tail) && !kis_marked(tail)) {
	    /* this will clear the marks and throw an error if the structure
	       is incorrect */
	    TValue entry = check_copy_single_entry(K, name, kcar(tail), obj);
	    krooted_tvs_push(K, entry);
	    TValue new_pair = kcons(K, entry, KNIL);
	    krooted_tvs_pop(K);
	    kmark(tail);
	    kset_cdr(last_pair, new_pair);
	    last_pair = new_pair;
	    tail = kcdr(tail);
	}

	/* dont close the cycle (if there is one) */
	unmark_list(K, obj);
	TValue ret = kcutoff_dummy1(K);
	if (!ttispair(tail) && !ttisnil(tail)) {
	    klispE_throw_simple(K, "expected list"); 
	    return KINERT;
	} 
	return ret;
    }
}

/* 7.2.4 guard-continuation */
void guard_continuation(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);

    bind_3tp(K, ptree, "any", anytype, entry_guards,
	     "continuation", ttiscontinuation, cont,
	     "any", anytype, exit_guards);

    entry_guards = check_copy_guards(K, "guard-continuation: entry guards", 
				     entry_guards);
    krooted_tvs_push(K, entry_guards);

    exit_guards = check_copy_guards(K, "guard-continuation: exit guards", 
				     exit_guards);
    krooted_tvs_push(K, exit_guards);

    TValue outer_cont = kmake_continuation(K, cont, do_pass_value, 
					   2, entry_guards, denv);
    krooted_tvs_push(K, outer_cont);
    /* mark it as an outer continuation */
    kset_outer_cont(outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
					   do_pass_value, 2, exit_guards, denv);
    /* mark it as an outer continuation */
    kset_inner_cont(inner_cont);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    kapply_cc(K, inner_cont);
}


/* 7.2.5 continuation->applicative */
void continuation_applicative(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "continuation",
	     ttiscontinuation, cont);
    /* cont_app is from kstate, it handles dynamic vars &
       interceptions */
    TValue app = kmake_applicative(K, cont_app, 1, cont);
    kapply_cc(K, app);
}

/* 7.2.6 root-continuation */
/* done in kground.c/krepl.c */

/* 7.2.7 error-continuation */
/* done in kground.c/krepl.c */

/* 
** 7.3 Library features
*/

/* 7.3.1 apply-continuation */
void apply_continuation(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_2tp(K, ptree, "continuation", ttiscontinuation,
	     cont, "any", anytype, obj);

    /* kcall_cont is from kstate, it handles dynamic vars &
       interceptions */
    kcall_cont(K, cont, obj);
}

/* 7.3.2 $let/cc */
void Slet_cc(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    /* from the report: #ignore is not ok, only symbol */
    bind_al1tp(K, ptree, "symbol", ttissymbol, sym, objs);

    if (ttisnil(objs)) {
	/* we don't even bother creating the environment */
	kapply_cc(K, KINERT);
    } else {
	TValue new_env = kmake_environment(K, denv);
	
	/* add binding may allocate, protect env, 
	  keep in stack until continuation is allocated */
	krooted_tvs_push(K, new_env); 
	kadd_binding(K, new_env, sym, kget_cc(K));
	
	/* the list of instructions is copied to avoid mutation */
	/* MAYBE: copy the evaluation structure, ASK John */
	TValue ls = check_copy_list(K, "$let/cc", objs, false);
        krooted_tvs_push(K, ls);

	/* this is needed because seq continuation doesn't check for 
	   nil sequence */
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K),
					     do_seq, 2, tail, new_env);
	    kset_cc(K, new_cont);
	} 

	krooted_tvs_pop(K); 
        krooted_tvs_pop(K);

	ktail_eval(K, kcar(ls), new_env);
    }
}

/* 7.3.3 guard-dynamic-extent */
void guard_dynamic_extent(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);

    bind_3tp(K, ptree, "any", anytype, entry_guards,
	     "combiner", ttiscombiner, comb,
	     "any", anytype, exit_guards);

    entry_guards = check_copy_guards(K, "guard-dynamic-extent: entry guards", 
				     entry_guards);
    krooted_tvs_push(K, entry_guards);
    exit_guards = check_copy_guards(K, "guard-dynamic-extent: exit guards", 
				     exit_guards);
    krooted_tvs_push(K, exit_guards);
    /* GC: root continuations */
    /* The current continuation is guarded */
    TValue outer_cont = kmake_continuation(K, kget_cc(K), do_pass_value, 
					   2, entry_guards, denv);
    kset_outer_cont(outer_cont);
    kset_cc(K, outer_cont); /* this implicitly roots outer_cont */

    TValue inner_cont = kmake_continuation(K, outer_cont, do_pass_value, 2, 
					   exit_guards, denv);
    kset_inner_cont(inner_cont);

    /* call combiner with no operands in the dynamic extent of inner,
     with the dynamic env of this call */
    kset_cc(K, inner_cont); /* this implicitly roots inner_cont */
    TValue expr = kcons(K, comb, KNIL);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, expr, denv);
}

/* 7.3.4 exit */    
/* Unlike in the report, in klisp this takes an optional argument
   to be passed to the root continuation (defaults to #inert) */
void kgexit(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

    TValue obj = ptree;
    if (!get_opt_tpar(K, obj, "any", anytype))
	obj = KINERT;

    /* TODO: look out for guards and dynamic variables */
    /* should be probably handled in kcall_cont() */
    kcall_cont(K, K->root_cont, obj);
}

/* init ground */
void kinit_continuations_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 7.1.1 continuation? */
    add_applicative(K, ground_env, "continuation?", typep, 2, symbol, 
		    i2tv(K_TCONTINUATION));
    /* 7.2.2 call/cc */
    add_applicative(K, ground_env, "call/cc", call_cc, 0);
    /* 7.2.3 extend-continuation */
    add_applicative(K, ground_env, "extend-continuation", extend_continuation, 
		    0);
    /* 7.2.4 guard-continuation */
    add_applicative(K, ground_env, "guard-continuation", guard_continuation, 
		    0);
    /* 7.2.5 continuation->applicative */
    add_applicative(K, ground_env, "continuation->applicative",
		    continuation_applicative, 0);
    /* 7.2.6 root-continuation */
    klisp_assert(ttiscontinuation(K->root_cont));
    add_value(K, ground_env, "root-continuation",
	      K->root_cont);
    /* 7.2.7 error-continuation */
    klisp_assert(ttiscontinuation(K->error_cont));
    add_value(K, ground_env, "error-continuation",
	      K->error_cont);
    /* 7.3.1 apply-continuation */
    add_applicative(K, ground_env, "apply-continuation", apply_continuation, 
		    0);
    /* 7.3.2 $let/cc */
    add_operative(K, ground_env, "$let/cc", Slet_cc, 
		    0);
    /* 7.3.3 guard-dynamic-extent */
    add_applicative(K, ground_env, "guard-dynamic-extent", 
		    guard_dynamic_extent, 0);
    /* 7.3.4 exit */    
    add_applicative(K, ground_env, "exit", kgexit, 
		    0);
}
