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
    bind_al2p(K, ptree, vptree, vpenv, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, "$vau", vptree, vpenv);
    
    krooted_tvs_push(K, vptree);

    /* the body should be a list */
    UNUSED(check_list(K, "$vau", true, vbody, NULL));
    vbody = copy_es_immutable_h(K, "$vau", vbody, false);

    krooted_tvs_push(K, vbody);
    
    TValue new_op = kmake_operative(K, do_vau, 4, vptree, vpenv, vbody, denv);

    #if KTRACK_SI
    /* save as source code info the info from the expression whose evaluation
       got us here */
    TValue si = kget_csi(K);
    if (!ttisnil(si)) {
	krooted_tvs_push(K, new_op);
	kset_source_info(K, new_op, si);
	krooted_tvs_pop(K);
    }
    #endif

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
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

    /* protect env */
    krooted_tvs_push(K, env); 

    /* TODO use name from operative */
    match(K, "[user-operative]", env, ptree, obj);
    if (!ttisignore(penv))
	kadd_binding(K, env, penv, denv);

    /* keep env in stack in case a cont has to be constructed */
    
    if (ttisnil(body)) {
	krooted_tvs_pop(K);
	kapply_cc(K, KINERT);
    } else {
	/* this is needed because seq continuation doesn't check for 
	   nil sequence */
	TValue tail = kcdr(body);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K),
					     do_seq, 2, tail, env);
	    kset_cc(K, new_cont);
#if KTRACK_SI
	    /* put the source info of the list including the element
	       that we are about to evaluate */
	    kset_source_info(K, new_cont, ktry_get_si(K, body));
#endif
	} 
	krooted_tvs_pop(K);
	ktail_eval(K, kcar(body), env);
    }
}

/* 4.10.4 wrap */
void wrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    bind_1tp(K, ptree, "combiner", ttiscombiner, comb);
    TValue new_app = kwrap(K, comb);
    #if KTRACK_SI
    /* save as source code info the info from the expression whose evaluation
       got us here */
    TValue si = kget_csi(K);
    if (!ttisnil(si)) {
	krooted_tvs_push(K, new_app);
	kset_source_info(K, new_app, si);
	krooted_tvs_pop(K);
    }
    #endif
    kapply_cc(K, new_app);
}

/* 4.10.5 unwrap */
void unwrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1tp(K, ptree, "applicative", ttisapplicative, app);
    TValue underlying = kunwrap(app);
    kapply_cc(K, underlying);
}

/* 5.3.1 $vau */
/* DONE: above, together with 4.10.4 */
/* 5.3.2 $lambda */
void Slambda(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    bind_al1p(K, ptree, vptree, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, "$lambda", vptree, KIGNORE);
    krooted_tvs_push(K, vptree); 
    /* the body should be a list */
    UNUSED(check_list(K, "$lambda", true, vbody, NULL));
    vbody = copy_es_immutable_h(K, "$lambda", vbody, false);

    krooted_tvs_push(K, vbody); 

    TValue new_app = kmake_applicative(K, do_vau, 4, vptree, KIGNORE, vbody, 
				       denv);
    #if KTRACK_SI
    /* save as source code info the info from the expression whose evaluation
       got us here, both for the applicative and the underlying combiner */
    TValue si = kget_csi(K);
    
    if (!ttisnil(si)) {
	krooted_tvs_push(K, new_app);
	kset_source_info(K, new_app, si);
	kset_source_info(K, kunwrap(new_app), si);
	krooted_tvs_pop(K);
    }
    #endif

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, new_app);
}

/* 5.5.1 apply */
void apply(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    bind_al2tp(K, ptree, 
	       "applicative", ttisapplicative, app, 
	       "any", anytype, obj, 
	       maybe_env);

    TValue env = (get_opt_tpar(K, "apply", K_TENVIRONMENT, &maybe_env))?
	maybe_env : kmake_empty_environment(K);

    krooted_tvs_push(K, env); 
    TValue expr = kcons(K, kunwrap(app), obj);
    krooted_tvs_pop(K); 
    /* TODO track source code info */
    ktail_eval(K, expr, env);
}

/* Helpers for map (also used by for each) */
void map_for_each_get_metrics(klisp_State *K, char *name, TValue lss,
			      int32_t *app_apairs_out, int32_t *app_cpairs_out,
			      int32_t *res_apairs_out, int32_t *res_cpairs_out)
{
    /* avoid warnings (shouldn't happen if _No_return was used in throw) */
    *app_apairs_out = 0;
    *app_cpairs_out = 0;
    *res_apairs_out = 0;
    *res_cpairs_out = 0;

    /* get the metrics of the ptree of each call to app */
    int32_t app_cpairs;
    int32_t app_pairs = check_list(K, name, true, lss, &app_cpairs);
    int32_t app_apairs = app_pairs - app_cpairs;

    /* get the metrics of the result list */
    int32_t res_cpairs;
    /* We now that lss has at least one elem */
    int32_t res_pairs = check_list(K, name, true, kcar(lss), &res_cpairs);
    int32_t res_apairs = res_pairs - res_cpairs;
    
    if (res_cpairs == 0) {
	/* finite list of length res_pairs (all lists should
	 have the same structure: acyclic with same length) */
	int32_t pairs = app_pairs - 1;
	TValue tail = kcdr(lss);
	while(pairs--) {
	    int32_t first_cpairs;
	    int32_t first_pairs = check_list(K, name, true, kcar(tail), 
					     &first_cpairs);
	    tail = kcdr(tail);

	    if (first_cpairs != 0) {
		klispE_throw_simple(K, "mixed finite and infinite lists");
		return;
	    } else if (first_pairs != res_pairs) {
		klispE_throw_simple(K, "lists of different length");
		return;
	    }
	}
    } else {
	/* cyclic list: all lists should be cyclic.
	   result will have acyclic length equal to the
	   max of all the lists and cyclic length equal to the lcm
	   of all the lists. res_pairs may be broken but will be 
	   restored by after the loop */
	int32_t pairs = app_pairs - 1;
	TValue tail = kcdr(lss);
	while(pairs--) {
	    int32_t first_cpairs;
	    int32_t first_pairs = check_list(K, name, true, kcar(tail), 
					     &first_cpairs);
	    int32_t first_apairs = first_pairs - first_cpairs;
	    tail = kcdr(tail);

	    if (first_cpairs == 0) {
		klispE_throw_simple(K, "mixed finite and infinite lists");
		return;
	    } 
	    res_apairs = kmax32(res_apairs, first_apairs);
	    /* this can throw an error if res_cpairs doesn't 
	       fit in 32 bits, which is a reasonable implementation
	       restriction because the list wouldn't fit in memory 
	       anyways */
	    res_cpairs = kcheck32(K, "map/for-each: result list is too big", 
				  klcm32_64(res_cpairs, first_cpairs));
	}
	res_pairs = kcheck32(K, "map/for-each: result list is too big", 
			     (int64_t) res_cpairs + (int64_t) res_apairs);
	UNUSED(res_pairs);
    }

    *app_apairs_out = app_apairs;
    *app_cpairs_out = app_cpairs;
    *res_apairs_out = res_apairs;
    *res_cpairs_out = res_cpairs;
}

/* Return two lists, isomorphic to lss: one list of cars and one list
   of cdrs (replacing the value of lss) */

/* GC: assumes lss is rooted, and dummy1 & 2 are free in K */
TValue map_for_each_get_cars_cdrs(klisp_State *K, TValue *lss, 
				  int32_t apairs, int32_t cpairs)
{
    TValue tail = *lss;

    TValue lp_cars = kget_dummy1(K);
    TValue lap_cars = lp_cars;

    TValue lp_cdrs = kget_dummy2(K);
    TValue lap_cdrs = lp_cdrs;
    
    while(apairs != 0 || cpairs != 0) {
	int32_t pairs;
	if (apairs != 0) {
	    pairs = apairs;
	} else {
	    /* remember last acyclic pair of both lists to to encycle! later */
	    lap_cars = lp_cars;
	    lap_cdrs = lp_cdrs;
	    pairs = cpairs;
	}

	while(pairs--) {
	    TValue first = kcar(tail);
	    tail = kcdr(tail);
	 
	    /* accumulate both cars and cdrs */
	    TValue np;
	    np = kcons(K, kcar(first), KNIL);
	    kset_cdr(lp_cars, np);
	    lp_cars = np;

	    np = kcons(K, kcdr(first), KNIL);
	    kset_cdr(lp_cdrs, np);
	    lp_cdrs = np;
	}

	if (apairs != 0) {
	    apairs = 0;
	} else {
	    cpairs = 0;
	    /* encycle! the list of cars and the list of cdrs */
	    TValue fcp, lcp;
	    fcp = kcdr(lap_cars);
	    lcp = lp_cars;
	    kset_cdr(lcp, fcp);

	    fcp = kcdr(lap_cdrs);
	    lcp = lp_cdrs;
	    kset_cdr(lcp, fcp);
	}
    }

    *lss = kcutoff_dummy2(K);
    return kcutoff_dummy1(K);
}

/* Transpose lss so that the result is a list of lists, each one having
   metrics (app_apairs, app_cpairs). The metrics of the returned list
   should be (res_apairs, res_cpairs) */

/* GC: assumes lss is rooted */
TValue map_for_each_transpose(klisp_State *K, TValue lss, 
			      int32_t app_apairs, int32_t app_cpairs, 
			      int32_t res_apairs, int32_t res_cpairs)
{
    /* reserve dummy1 & 2 to get_cars_cdrs */
    TValue lp = kget_dummy3(K);
    TValue lap = lp;

    TValue cars = KNIL; /* put something for GC */
    TValue tail = lss;

    /* GC: both cars & tail vary in each loop, to protect them we need
       the vars stack */
    krooted_vars_push(K, &cars);
    krooted_vars_push(K, &tail);
    
    /* Loop over list of lists, creating a list of cars and 
       a list of cdrs, accumulate the list of cars and loop
       with the list of cdrs as the new list of lists (lss) */
    while(res_apairs != 0 || res_cpairs != 0) {
	int32_t pairs;
	
	if (res_apairs != 0) {
	    pairs = res_apairs;
	} else {
	    pairs = res_cpairs;
	    /* remember last acyclic pair to encycle! later */
	    lap = lp;
	}

	while(pairs--) {
	    /* accumulate cars and replace tail with cdrs */
	    cars = map_for_each_get_cars_cdrs(K, &tail, app_apairs, app_cpairs);
	    TValue np = kcons(K, cars, KNIL);
	    kset_cdr(lp, np);
	    lp = np;
	}

	if (res_apairs != 0) {
	    res_apairs = 0;
	} else {
	    res_cpairs = 0;
	    /* encycle! the list of list of cars */
	    TValue fcp = kcdr(lap);
	    TValue lcp = lp;
	    kset_cdr(lcp, fcp);
	}
    }

    krooted_vars_pop(K);
    krooted_vars_pop(K);
    return kcutoff_dummy3(K);
}

/* Continuation helpers for map */

/* For acyclic input lists: Return the mapped list */
void do_map_ret(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: (dummy . complete-ls)
    */
    UNUSED(obj);
    /* copy the list to avoid problems with continuations
       captured from within the dynamic extent to map
       and later mutation of the result */
    /* XXX: the check isn't necessary really, but there is
       no list_copy */
    TValue copy = check_copy_list(K, "map", kcdr(xparams[0]), false);
    kapply_cc(K, copy);
}

/* For cyclic input list: close the cycle and return the mapped list */
void do_map_encycle(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: (dummy . complete-ls)
    ** xparams[1]: last non-cycle pair
    */
    /* obj: (rem-ls . last-pair) */
    TValue lp = kcdr(obj);
    TValue lap = xparams[1];

    TValue fcp = kcdr(lap);
    TValue lcp = lp;
    kset_cdr(lcp, fcp);

    /* copy the list to avoid problems with continuations
       captured from within the dynamic extent to map
       and later mutation of the result */
    /* XXX: the check isn't necessary really, but there is
       no list_copy */
    TValue copy = check_copy_list(K, "map", kcdr(xparams[0]), false);
    kapply_cc(K, copy);
}

void do_map(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: app
    ** xparams[1]: rem-ls
    ** xparams[2]: last-pair
    ** xparams[3]: n
    ** xparams[4]: denv
    ** xparams[5]: dummyp
    */
    TValue app = xparams[0];
    TValue ls = xparams[1];
    TValue last_pair = xparams[2];
    int32_t n = ivalue(xparams[3]);
    TValue denv = xparams[4];
    bool dummyp = bvalue(xparams[5]);

    /* this case is used to kick start the mapping of both
       the acyclic and cyclic part, avoiding code duplication */
    if (!dummyp) {
	TValue np = kcons(K, obj, KNIL);
	kset_cdr(last_pair, np);
	last_pair = np;
    }

    if (n == 0) {
        /* pass the rest of the list and last pair for cycle handling */
	kapply_cc(K, kcons(K, ls, last_pair));
    } else {
	/* copy the ptree to avoid problems with mutation */
	/* XXX: no check necessary, could just use copy_list if there
	 was such a procedure */
	TValue first_ptree = check_copy_list(K, "map", kcar(ls), false);
	ls = kcdr(ls);
	n = n-1;
	krooted_tvs_push(K, first_ptree);
	/* have to unwrap the applicative to avoid extra evaluation of first */
	TValue new_expr = kcons(K, kunwrap(app), first_ptree);
	krooted_tvs_push(K, new_expr);
	TValue new_cont = 
	    kmake_continuation(K, kget_cc(K), do_map, 6, app, 
			       ls, last_pair, i2tv(n), denv, KFALSE);
	krooted_tvs_pop(K); 
	krooted_tvs_pop(K); 
	kset_cc(K, new_cont);
	ktail_eval(K, new_expr, denv);
    }
}

void do_map_cycle(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: app
    ** xparams[1]: (dummy . res-list)
    ** xparams[2]: cpairs
    ** xparams[3]: denv
    */ 

    TValue app = xparams[0];
    TValue dummy = xparams[1];
    int32_t cpairs = ivalue(xparams[2]);
    TValue denv = xparams[3];

    /* obj: (cycle-part . last-result-pair) */
    TValue ls = kcar(obj);
    TValue last_apair = kcdr(obj);

    /* this continuation will close the cycle and return the list */
    TValue encycle_cont =
 	kmake_continuation(K, kget_cc(K), do_map_encycle, 2, 
			   dummy, last_apair);

    krooted_tvs_push(K, encycle_cont);
    /* schedule the mapping of the elements of the cycle, 
       signal dummyp = true to avoid creating a pair for
       the inert value passed to the first continuation */
    TValue new_cont = 
	kmake_continuation(K, encycle_cont, do_map, 6, app, ls, 
			   last_apair, cpairs, denv, KTRUE);
    krooted_tvs_pop(K); 
    kset_cc(K, new_cont);
    /* this will be like a nop and will continue with do_map */
    kapply_cc(K, KINERT);
}

/* 5.9.1 map */
void map(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);

    bind_al1tp(K, ptree, "applicative", ttisapplicative, app, lss);
    
    if (ttisnil(lss)) {
	klispE_throw_simple(K, "no lists");
	return;
    }

    /* get the metrics of the ptree of each call to app and
       of the result list */
    int32_t app_pairs, app_apairs, app_cpairs;
    int32_t res_pairs, res_apairs, res_cpairs;

    map_for_each_get_metrics(K, "map", lss, &app_apairs, &app_cpairs,
			     &res_apairs, &res_cpairs);
    app_pairs = app_apairs + app_cpairs;
    res_pairs = res_apairs + res_cpairs;

    /* create the list of parameters to app */
    lss = map_for_each_transpose(K, lss, app_apairs, app_cpairs, 
				 res_apairs, res_cpairs);

    /* ASK John: the semantics when this is mixed with continuations,
       isn't all that great..., but what are the expectations considering
       there is no prescribed order? */

    krooted_tvs_push(K, lss);
    /* This will be the list to be returned, but it will be copied
       before to play a little nicer with continuations */
    TValue dummy = kcons(K, KINERT, KNIL);
    
    krooted_tvs_push(K, dummy);

    TValue ret_cont = (res_cpairs == 0)?
	kmake_continuation(K, kget_cc(K), do_map_ret, 1, dummy)
	: kmake_continuation(K, kget_cc(K), do_map_cycle, 4, 
			     app, dummy, i2tv(res_cpairs), denv);


    krooted_tvs_push(K, ret_cont);

    /* schedule the mapping of the elements of the acyclic part.
       signal dummyp = true to avoid creating a pair for
       the inert value passed to the first continuation */
    TValue new_cont = 
	kmake_continuation(K, ret_cont, do_map, 6, app, lss, dummy,
			   i2tv(res_apairs), denv, KTRUE);

    krooted_tvs_pop(K); 
    krooted_tvs_pop(K); 
    krooted_tvs_pop(K); 

    kset_cc(K, new_cont);
    /* this will be a nop, and will continue with do_map */
    kapply_cc(K, KINERT);
}

/* 6.2.1 combiner? */
/* uses ftypedp */

/* Helper for combiner? */
bool kcombinerp(TValue obj) { return ttiscombiner(obj); }
