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
void call_cc(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
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
    TValue underlying = kunwrap(app);
    TValue env = xparams[1];

    TValue expr = kcons(K, underlying, obj);
    ktail_eval(K, expr, env);
}

/* 7.2.3 extend-continuation */
void extend_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

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

/* Helpers for guard-continuation (& guard-dynamic-extent) */

/* this is used for inner & outer continuations, it just
   passes the value. xparams is not actually empty, it contains
   the entry/exit guards, but they are used only in 
   continuation->applicative (that is during abnormal passes) */
void do_pass_value(klisp_State *K, TValue *xparams, TValue obj)
{
    UNUSED(xparams);
    kapply_cc(K, obj);
}

#define singly_wrapped(obj_) (ttisapplicative(obj_) && \
			      ttisoperative(kunwrap(obj_)))

/* this unmarks root before throwing any error */
/* TODO: this isn't very clean, refactor */
inline TValue check_copy_single_entry(klisp_State *K, char *name,
				      TValue obj, TValue root)
{
    if (!ttispair(obj) || !ttispair(kcdr(obj)) || 
	    !ttisnil(kcddr(obj))) {
	unmark_list(K, root);
	klispE_throw_extra(K, name , ": Bad entry (expected "
			   "list of length 2)");
	return KINERT;
    } 
    TValue cont = kcar(obj);
    TValue app = kcadr(obj);

    if (!ttiscontinuation(cont)) {
	unmark_list(K, root);
	klispE_throw_extra(K, name, ": Bad type on first element (expected " 
		     "continuation)");				     
	return KINERT;
    } else if (!singly_wrapped(app)) { 
	unmark_list(K, root);
	klispE_throw_extra(K, name, ": Bad type on second element (expected " 
		     "singly wrapped applicative)");				     
	return KINERT; 
    }

    /* GC: save intermediate pair */
    /* save the operative directly, don't waste space/time
     with a list, use just a pair */
    return kcons(K, cont, kunwrap(app)); 
}

/* the guards are probably generated on the spot so we don't check
   for immutability and copy it anyways */
TValue check_copy_guards(klisp_State *K, char *name, TValue obj)
{
    if (ttisnil(obj)) {
	return obj;
    } else {
	TValue dummy = kcons(K, KINERT, KNIL);
	TValue last_pair = dummy;
	TValue tail = obj;
    
	while(ttispair(tail) && !kis_marked(tail)) {
	    /* this will clear the marks and throw an error if the structure
	       is incorrect */
	    TValue entry = check_copy_single_entry(K, name, kcar(tail), obj);
	    TValue new_pair = kcons(K, entry, KNIL);
	    kmark(tail);
	    kset_cdr(last_pair, new_pair);
	    last_pair = new_pair;
	    tail = kcdr(tail);
	}

	/* dont close the cycle (if there is one) */
	unmark_list(K, obj);

	if (!ttispair(tail) && !ttisnil(tail)) {
	    klispE_throw_extra(K, name , ": expected list"); 
	    return KINERT;
	} 
	return kcdr(dummy);
    }
}

/* 7.2.4 guard-continuation */
void guard_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv)
{
    UNUSED(xparams);

    bind_3tp(K, "guard-continuation", ptree, "any", anytype, entry_guards,
	     "continuation", ttiscontinuation, cont,
	     "any", anytype, exit_guards);

    entry_guards = check_copy_guards(K, "guard-continuation: entry guards", 
				     entry_guards);
    exit_guards = check_copy_guards(K, "guard-continuation: exit guards", 
				     exit_guards);

    TValue outer_cont = kmake_continuation(K, cont, KNIL, KNIL, do_pass_value, 
					   2, entry_guards, denv);
    /* mark it as an outer continuation */
    kset_outer_cont(outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, KNIL, KNIL, 
					   do_pass_value, 2, exit_guards, denv);
    /* mark it as an outer continuation */
    kset_inner_cont(inner_cont);
    kapply_cc(K, inner_cont);
}


/* 7.2.5 continuation->applicative */
/* TODO: look out for guards and dynamic variables */
void continuation_applicative(klisp_State *K, TValue *xparams, TValue ptree, 
			      TValue denv)
{
    UNUSED(xparams);
    bind_1tp(K, "continuation->applicative", ptree, "continuation",
	     ttiscontinuation, cont);
    /* cont_app is from kstate */
    TValue app = make_applicative(K, cont_app, 1, cont);
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
void apply_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_2tp(K, "apply-continuation", ptree, "continuation", ttiscontinuation,
	     cont, "any", anytype, obj);

    /* TODO: look out for guards and dynamic variables */
    /* should be probably handled in kcall_cont() */
    kcall_cont(K, cont, obj);
}

/* 7.3.2 $let/cc */
void Slet_cc(klisp_State *K, TValue *xparams, TValue ptree, 
	     TValue denv)
{
    UNUSED(xparams);
    /* from the report: #ignore is not ok, only symbol */
    bind_al1tp(K, "$let/cc", ptree, "symbol", ttissymbol, sym, objs);

    if (ttisnil(objs)) {
	/* we don't even bother creating the environment */
	kapply_cc(K, KINERT);
    } else {
	TValue new_env = kmake_environment(K, denv);
	
	/* add binding may allocate, protect env */
	krooted_tvs_push(K, new_env); 
	kadd_binding(K, new_env, sym, kget_cc(K));
	
	/* the list of instructions is copied to avoid mutation */
	/* MAYBE: copy the evaluation structure, ASK John */
	TValue ls = check_copy_list(K, "$let/cc", objs, false);

	krooted_tvs_pop(K); /* make cont will protect it now */

	/* this is needed because seq continuation doesn't check for 
	   nil sequence */
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, new_env);
	    kset_cc(K, new_cont);
	} 
	ktail_eval(K, kcar(ls), new_env);
    }
}

/* 7.3.3 guard-dynamic-extent */
void guard_dynamic_extent(klisp_State *K, TValue *xparams, TValue ptree, 
			  TValue denv)
{
    UNUSED(xparams);

    bind_3tp(K, "guard-dynamic-extent", ptree, "any", anytype, entry_guards,
	     "combiner", ttiscombiner, comb,
	     "any", anytype, exit_guards);

    entry_guards = check_copy_guards(K, "guard-dynamic-extent: entry guards", 
				     entry_guards);
    exit_guards = check_copy_guards(K, "guard-dynamic-extent: exit guards", 
				     exit_guards);
    /* GC: root continuations */
    /* The current continuation is guarded */
    TValue outer_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_pass_value, 
					   1, entry_guards);
    kset_outer_cont(outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, KNIL, KNIL, 
					   do_pass_value, 1, exit_guards);
    kset_inner_cont(inner_cont);

    /* call combiner with no operands in the dynamic extent of inner,
     with the dynamic env of this call */
    kset_cc(K, inner_cont);
    TValue expr = kcons(K, comb, KNIL);
    ktail_eval(K, expr, denv);
}

/* 7.3.4 exit */    
void kgexit(klisp_State *K, TValue *xparams, TValue ptree, 
	    TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    check_0p(K, "exit", ptree);

    /* TODO: look out for guards and dynamic variables */
    /* should be probably handled in kcall_cont() */
    kcall_cont(K, K->root_cont, KINERT);
}
