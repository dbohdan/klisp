/*
** kgpairs_lists.c
** Pairs and lists features for the ground environment
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
#include "kstring.h"
#include "kcontinuation.h"
#include "kenvironment.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgequalp.h"
#include "kgpairs_lists.h"
#include "kgnumbers.h"
#include "kinteger.h"

/* 4.6.1 pair? */
/* uses typep */

/* 4.6.2 null? */
/* uses typep */
    
/* 4.6.3 cons */
void cons(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2p(K, "cons", ptree, car, cdr);
    
    TValue new_pair = kcons(K, car, cdr);
    kapply_cc(K, new_pair);
}


/* 5.2.1 list */
void list(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
/* the underlying combiner of list return the complete ptree, the only list
   checking is implicit in the applicative evaluation */
    (void) xparams;
    (void) denv;
    kapply_cc(K, ptree);
}

/* 5.2.2 list* */
void listS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
/* TODO: 
   OPTIMIZE: if this call is a result of a call to eval, we could get away
   with just setting the kcdr of the next to last pair to the car of
   the last pair, because the list of operands is fresh. Also the type
   check wouldn't be necessary. This optimization technique could be
   used in lots of places to avoid checks and the like. */
    (void) xparams;
    (void) denv;

    if (ttisnil(ptree)) {
	klispE_throw(K, "list*: empty argument list"); 
	return;
    }
    /* GC: should root dummy */
    TValue dummy = kcons(K, KINERT, KNIL);
    TValue last_pair = dummy;
    TValue tail = ptree;
    
    /* First copy the list, but remembering the next to last pair */
    while(ttispair(tail) && !kis_marked(tail)) {
	kmark(tail);
	/* we save the next_to last pair in the cdr to 
	   allow the change into an improper list later */
	TValue new_pair = kcons(K, kcar(tail), last_pair);
	kset_cdr(last_pair, new_pair);
	last_pair = new_pair;
	tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttisnil(tail)) {
	/* Now eliminate the last pair to get the correct improper list.
	   This avoids an if in the above loop. It's inside the if because
	   we need at least one pair for this to work. */
	TValue next_to_last_pair = kcdr(last_pair);
	kset_cdr(next_to_last_pair, kcar(last_pair));
	kapply_cc(K, kcdr(dummy));
    } else if (ttispair(tail)) { /* cyclic argument list */
	klispE_throw(K, "list*: cyclic argument list"); 
	return;
    } else {
	klispE_throw(K, "list*: argument list is improper"); 
	return;
    }
}

/* 5.4.1 car, cdr */
/* 5.4.2 caar, cadr, ... cddddr */

void c_ad_r( klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{

    /*
    ** xparams[0]: name as symbol
    ** xparams[1]: an int with the less significant 2 nibbles 
    **             standing for the count and the branch selection.
    **             The high nibble is the count: that is the number of
    **             'a's and 'd's in the name, for example:
    **             0x1? for car and cdr.
    **             0x2? for caar, cadr, cdar and cddr.
    **             The low nibble is the branch selection, a 0 bit means
    **             car, a 1 bit means cdr, the first bit to be applied 
    **             is bit 0 so: caar=0x20, cadr=0x21, cdar:0x22, cddr 0x23
    */

    char *name = ksymbol_buf(xparams[0]);
    int p = ivalue(xparams[1]);
    int count = (p >> 4) & 0xf;
    int branches = p & 0xf;

    bind_1p(K, name, ptree, obj);

    while(count) {
	if (!ttispair(obj)) {
	    klispE_throw_extra(K, name, ": non pair found while traversing"); 
	    return;
	}
	obj = ((branches & 1) == 0)? kcar(obj) : kcdr(obj);
	branches >>= 1;
	--count;
    }
    kapply_cc(K, obj);
}

/* also used in list-tail and list-ref when receiving
   bigint indexes */
void get_list_metrics_aux(klisp_State *K, TValue obj, int32_t *p, int32_t *n, 
			  int32_t *a, int32_t *c)
{
    TValue tail = obj;
    int32_t pairs = 0;

    while(ttispair(tail) && !kis_marked(tail)) {
	/* record the pair number to simplify cycle pair counting */
	kset_mark(tail, i2tv(pairs));
	++pairs;
	tail = kcdr(tail);
    }
    int32_t apairs, cpairs, nils;
    if (ttisnil(tail)) {
	/* simple (possibly empty) list */
	apairs = pairs;
	nils = 1;
	cpairs = 0;
    } else if (ttispair(tail)) {
	/* cyclic (maybe circular) list */
	apairs = ivalue(kget_mark(tail));
	cpairs = pairs - apairs;
	nils = 0;
    } else {
	apairs = pairs;
	cpairs = 0;
	nils = 0;
    }

    unmark_list(K, obj);

    if (p) *p = pairs;
    if (n) *n = nils;
    if (a) *a = apairs;
    if (c) *c = cpairs;
}

/* 5.7.1 get-list-metrics */
void get_list_metrics(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_1p(K, "get-list-metrics", ptree, obj);

    int32_t pairs, nils, apairs, cpairs;
    get_list_metrics_aux(K, obj, &pairs, &nils, &apairs, &cpairs);

    /* GC: root intermediate pairs */
    TValue res = kcons(K, i2tv(apairs), kcons(K, i2tv(cpairs), KNIL));
    res = kcons(K, i2tv(pairs), kcons(K, i2tv(nils), res));
    kapply_cc(K, res);
}

/* Helper for list-tail and list-ref */

/* Calculate the smallest i such that 
   (eq? (list-tail obj i) (list-tail obj tk))
   tk is a bigint and all lists have fixint range number of pairs,
   so the list should cyclic and we should calculate an index that
   doesn't go through the complete cycle not even once */
int32_t ksmallest_index(klisp_State *K, char *name, TValue obj, 
		      TValue tk)
{
    int32_t apairs, cpairs;
    get_list_metrics_aux(K, obj, NULL, NULL, &apairs, &cpairs);
    if (cpairs == 0) {
	klispE_throw_extra(K, name, ": non pair found while traversing "
			   "object");
	return 0;
    }
    TValue tv_apairs = i2tv(apairs);
    TValue tv_cpairs = i2tv(cpairs);
	
    /* all calculations will be done with bigints */
    kensure_bigint(tv_apairs);
    kensure_bigint(tv_cpairs);
	
    TValue idx = kbigint_minus(K, tk, tv_apairs);
    /* idx may have become a fixint */
    kensure_bigint(idx);
    UNUSED(kbigint_div_mod(K, idx, tv_cpairs, &idx));
    /* now idx is less than cpairs so it fits in a fixint */
    assert(ttisfixint(idx));
    return ivalue(idx) + apairs; 
}


/* 5.7.2 list-tail */
void list_tail(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
/* ASK John: can the object be a cyclic list? the wording of the report
   seems to indicate that can't be the case, but it makes sense here 
   (cf $encycle!) to allow cyclic lists, so that's what I do */
    (void) denv;
    (void) xparams;
    bind_2tp(K, "list-tail", ptree, "any", anytype, obj,
	     "finite integer", kintegerp, tk);

    if (knegativep(tk)) {
	klispE_throw(K, "list-tail: negative index");
	return;
    }

    int32_t k = (ttisfixint(tk))? ivalue(tk)
	: ksmallest_index(K, "list-tail", obj, tk);

    while(k) {
	if (!ttispair(obj)) {
	    klispE_throw(K, "list-tail: non pair found while traversing "
			 "object");
	    return;
	}
	obj = kcdr(obj);
	--k;
    }
    kapply_cc(K, obj);
}

/* 6.3.1 length */
void length(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1p(K, "length", ptree, obj);

    TValue tail = obj;
    int pairs = 0;
    while(ttispair(tail) && !kis_marked(tail)) {
	kmark(tail);
	tail = kcdr(tail);
	++pairs;
    }
    unmark_list(K, obj);

    TValue res = ttispair(tail)? KEPINF : i2tv(pairs);
    kapply_cc(K, res);
}

/* 6.3.2 list-ref */
void list_ref(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
/* ASK John: can the object be an improper list? the wording of the report
   seems to indicate that can't be the case, but it makes sense 
   (cf list-tail) For now we allow it. */
    UNUSED(denv);
    UNUSED(xparams);

    bind_2tp(K, "list-ref", ptree, "any", anytype, obj,
	     "finite integer", kintegerp, tk);

    if (knegativep(tk)) {
	klispE_throw(K, "list-ref: negative index");
	return;
    }

    int32_t k = (ttisfixint(tk))? ivalue(tk)
	: ksmallest_index(K, "list-tail", obj, tk);

    while(k) {
	if (!ttispair(obj)) {
	    klispE_throw(K, "list-ref: non pair found while traversing "
			 "object");
	    return;
	}
	obj = kcdr(obj);
	--k;
    }
    if (!ttispair(obj)) {
	klispE_throw(K, "list-ref: non pair found while traversing "
		     "object");
	return;
    }
    TValue res = kcar(obj);
    kapply_cc(K, res);
}

/* Helper for append */

/* Check that ls is an acyclic list, copy it and return both the list
   (as the ret value) and the last_pair. If obj is nil, *last_pair remains
   unmodified (this avoids having to check ttisnil before calling this) */
TValue append_check_copy_list(klisp_State *K, char *name, TValue obj, 
			      TValue *last_pair_ptr)
{
    /* return early if nil to avoid setting *last_pair_ptr */
    if (ttisnil(obj))
	return obj;

    TValue dummy = kcons(K, KINERT, KNIL);
    TValue last_pair = dummy;
    TValue tail = obj;
    
    while(ttispair(tail) && !kis_marked(tail)) {
	kmark(tail);
	TValue new_pair = kcons(K, kcar(tail), KNIL);
	kset_cdr(last_pair, new_pair);
	last_pair = new_pair;
	tail = kcdr(tail);
    }
    unmark_list(K, obj);

    if (ttispair(tail)) {
	klispE_throw_extra(K, name , ": expected acyclic list"); 
	return KINERT;
    } else if (!ttisnil(tail)) {
	klispE_throw_extra(K, name , ": expected list"); 
	return KINERT;
    }
    *last_pair_ptr = last_pair;
    return kcdr(dummy);
}

/* 6.3.3 append */
void append(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    int32_t cpairs;
    int32_t pairs = check_list(K, "append", true, ptree, &cpairs);
    int32_t apairs = pairs - cpairs;

    TValue dummy = kcons(K, KINERT, KNIL);
    TValue last_pair = dummy;
    TValue lss = ptree;
    TValue last_apair;

    while (apairs != 0 || cpairs != 0) {
	if (apairs == 0) {
	    /* this is the first run of the loop (if there is no acyclic part) 
	       or the second run of the loop (the cyclic part), 
	       must remember the last acyclic pair to encycle! the result */
	    last_apair = last_pair;
	    pairs = cpairs;
	} else {
	    /* this is the first (maybe only) run of the loop 
	       (the acyclic part) */
	    pairs = apairs;
	}

	while (pairs--) {
	    TValue first = kcar(lss);
	    lss = kcdr(lss);
	    TValue next_list;
	    TValue new_last_pair = last_pair; /* this helps if first is nil */
	    /* don't check or copy last list */
	    if (ttisnil(lss)) {
		/* here, new_last_pair is bogus, but it isn't necessary 
		 anymore so don't set it */
		next_list = first;
	    } else {
		next_list = append_check_copy_list(K, "append", first, 
						   &new_last_pair);
	    }
	    kset_cdr(last_pair, next_list);
	    last_pair = new_last_pair;
	}

	if (apairs != 0) {
	    /* acyclic part done */
	    apairs = 0;
	} else {
	    /* cyclic part done */
	    cpairs = 0;
	    TValue first_cpair = kcdr(last_apair);
	    TValue last_cpair = last_pair;
	    /* this works even if there is no cycle to be formed
	       (kcdr(last_apair) == ()) */
	    kset_cdr(last_cpair, first_cpair); /* encycle! */
	}
    }
    kapply_cc(K, kcdr(dummy));
}

/* 6.3.4 list-neighbors */
void list_neighbors(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    /* GC: root intermediate pairs */
    bind_1p(K, "list_neighbors", ptree, ls);

    int32_t cpairs;
    int32_t pairs = check_list(K, "list_neighbors", true, ls, &cpairs);

    TValue tail = ls;
    int32_t count = cpairs? pairs - cpairs : pairs - 1;
    TValue dummy = kcons(K, KINERT, KNIL);
    TValue last_pair = dummy;
    TValue last_apair = dummy; /* set after first loop */
    bool doing_cycle = false;

    while(count > 0 || !doing_cycle) {
	while(count-- > 0) { /* can be -1 if ls is nil */
	    TValue first = kcar(tail);
	    tail = kcdr(tail); /* tail advances one place per iter */
	    TValue new_car = kcons(K, first, kcons(K, kcar(tail), KNIL));
	    TValue new_pair = kcons(K, new_car, KNIL);
	    kset_cdr(last_pair, new_pair);
	    last_pair = new_pair;
	}

	if (doing_cycle) {
	    TValue first_cpair = kcdr(last_apair);
	    kset_cdr(last_pair, first_cpair);
	} else { /* this is done even if cpairs is 0 to terminate the loop */
	    doing_cycle = true;
	    /* must remember first cycle pair to reconstruct the cycle,
	       we can save the last outside of the cycle and then check 
	       its cdr */
	    last_apair = last_pair;
	    count = cpairs; /* this contains the sublist that has the last
			       and first element of the cycle */
	    /* this will loop once more */
	}
    }
    /* discard dummy pair to obtain the constructed list */
    kapply_cc(K, kcdr(dummy));
}

/* Helpers for filter */

/* For acyclic input lists: Return the filtered list */
void do_ret_cdr(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: (dummy . complete-ls)
    */
    UNUSED(obj);
    /* copy the list to avoid problems with continuations
       captured from within the dynamic extent to filter
       and later mutation of the result */
    /* XXX: the check isn't necessary really, but there is
       no list_copy (and if there was it would take apairs and
       cpairs, which we don't have here */
    TValue copy = check_copy_list(K, "filter", kcdr(xparams[0]), true);
    kapply_cc(K, copy);
}

/* For cyclic input list: If the result cycle is non empty, 
   close it and return filtered list */
void do_filter_encycle(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: (dummy . complete-ls)
    ** xparams[1]: last non-cycle pair
    */
    /* obj: (rem-ls . last-pair) */
    TValue last_pair = kcdr(obj);
    TValue last_non_cycle_pair = xparams[1];

    if (tv_equal(last_pair, last_non_cycle_pair)) {
	/* no cycle in result, this isn't strictly necessary
	   but just in case */
	kset_cdr(last_non_cycle_pair, KNIL);
    } else {
	/* There are pairs in the cycle, so close it */
	TValue first_cycle_pair = kcdr(last_non_cycle_pair);
	TValue last_cycle_pair = last_pair;
	kset_cdr(last_cycle_pair, first_cycle_pair);
    }

    /* copy the list to avoid problems with continuations
       captured from within the dynamic extent to filter
       and later mutation of the result */
    /* XXX: the check isn't necessary really, but there is
       no list_copy (and if there was it would take apairs and
       cpairs, which we don't have here */
    TValue copy = check_copy_list(K, "filter", kcdr(xparams[0]), true);
    kapply_cc(K, copy);
}

void do_filter(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: app
    ** xparams[1]: (last-obj . rem-ls)
    ** xparams[2]: last-pair in result list
    ** xparams[3]: n
    */
    TValue app = xparams[0];
    TValue ls = xparams[1];
    TValue last_obj = kcar(ls);
    ls = kcdr(ls);
    TValue last_pair = xparams[2];
    int32_t n = ivalue(xparams[3]);

    if (!ttisboolean(obj)) {
	klispE_throw(K, "filter: expected boolean result");
	return;
    } 
    
    if (kis_true(obj)) {
	TValue np = kcons(K, last_obj, KNIL);
	kset_cdr(last_pair, np);
	last_pair = np;
    }

    if (n == 0) {
        /* pass the rest of the list and last pair for cycle handling */
	kapply_cc(K, kcons(K, ls, last_pair)); 
    } else {
	TValue new_n = i2tv(n-1);
	TValue first = kcar(ls);
	TValue new_env = kmake_empty_environment(K);
	/* have to unwrap the applicative to avoid extra evaluation of first */
	TValue new_expr = kcons(K, kunwrap(app), kcons(K, first, KNIL));
	TValue new_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_filter, 4, app, 
			       ls, last_pair, new_n);
	kset_cc(K, new_cont);
	ktail_eval(K, new_expr, new_env);
    }
}

void do_filter_cycle(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: app
    ** xparams[1]: (dummy . res-list)
    ** xparams[2]: cpairs
    */ 

    TValue app = xparams[0];
    TValue dummy = xparams[1];
    TValue cpairs = xparams[2];

    /* obj: (cycle-part . last-result-pair) */
    TValue ls = kcar(obj);
    TValue last_apair = kcdr(obj);

    /* this continuation will close the cycle and return the list */
    TValue encycle_cont =
 	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_filter_encycle, 2, 
			   dummy, last_apair);

    /* schedule the filtering of the elements of the cycle */
    /* add inert before first element to be discarded when KFALSE 
       is received */
    TValue new_cont = 
	kmake_continuation(K, encycle_cont, KNIL, KNIL, do_filter, 4, app, 
			   kcons(K, KINERT, ls), last_apair, cpairs);
    kset_cc(K, new_cont);
    /* this will be like a nop and will continue with do_filter */
    kapply_cc(K, KFALSE);
}

/* 6.3.5 filter */
void filter(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, "filter", ptree, "applicative", ttisapplicative, app,
	     "any", anytype, ls);
    /* copy the list to ignore changes made by the applicative */
    /* REFACTOR: do this in a single pass */
    /* ASK John: the semantics when this is mixed with continuations,
       isn't all that great..., but what are the expectations considering
       there is no prescribed order? */
    int32_t cpairs;
    int32_t pairs = check_list(K, "filter", true, ls, &cpairs);
    /* XXX: This was the paradigmatic use case of the force copy flag
       in the old implementation, but it caused problems with continuations
       Is there any other use case for force copy flag?? */
    ls = check_copy_list(K, "filter", ls, false);
    /* This will be the list to be returned, but it will be copied
       before to play a little nicer with continuations */
    TValue dummy = kcons(K, KINERT, KNIL);
    
    TValue ret_cont = (cpairs == 0)?
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_ret_cdr, 1, dummy)
	: kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_filter_cycle, 3, 
			     app, dummy, i2tv(cpairs));
    /* add inert before first element to be discarded when KFALSE 
       is received */
    TValue new_cont = 
	kmake_continuation(K, ret_cont, KNIL, KNIL, do_filter, 4, app, 
			   kcons(K, KINERT, ls), dummy, i2tv(pairs-cpairs));
    kset_cc(K, new_cont);
    /* this will be a nop, and will continue with do_filter */
    kapply_cc(K, KFALSE);
}

/* 6.3.6 assoc */
void assoc(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_2p(K, "assoc", ptree, obj, ls);
    /* first pass, check structure */
    int32_t dummy;
    int32_t pairs = check_typed_list(K, "assoc", "pair", kpairp,
				     true, ls, &dummy);
    TValue tail = ls;
    TValue res = KNIL;
    while(pairs--) {
	TValue first = kcar(tail);
	if (equal2p(K, kcar(first), obj)) {
	    res = first;
	    break;
	}
	tail = kcdr(tail);
    }

    kapply_cc(K, res);
}

/* 6.3.7 member? */
void memberp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_2p(K, "member?", ptree, obj, ls);
    /* first pass, check structure */
    int32_t dummy;
    int32_t pairs = check_list(K, "member?", true, ls, &dummy);
    TValue tail = ls;
    TValue res = KFALSE;
    while(pairs--) {
	TValue first = kcar(tail);
	if (equal2p(K, first, obj)) {
	    res = KTRUE;
	    break;
	}
	tail = kcdr(tail);
    }

    kapply_cc(K, res);
}

/* 6.3.8 finite-list? */
/* NOTE: can't use ftypep because the predicate marks pairs too */
void finite_listp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    int32_t dummy;
    int32_t pairs = check_list(K, "finite-list?", true, ptree, &dummy);

    TValue res = KTRUE;
    TValue tail = ptree;
    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	TValue itail = first;
	while(ttispair(itail) && !kis_marked(itail)) {
	    kmark(itail);
	    itail = kcdr(itail);
	}
	unmark_list(K, first);
	
	if (!ttisnil(itail)) {
	    res = KFALSE;
	    break;
	}
    }
    kapply_cc(K, res);
}

/* 6.3.9 countable-list? */
/* NOTE: can't use ftypep because the predicate marks pairs too */
void countable_listp(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    int32_t dummy;
    int32_t pairs = check_list(K, "countable-list?", true, ptree, &dummy);

    TValue res = KTRUE;
    TValue tail = ptree;
    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	TValue itail = first;
	while(ttispair(itail) && !kis_marked(itail)) {
	    kmark(itail);
	    itail = kcdr(itail);
	}
	unmark_list(K, first);
	
	if (!ttisnil(itail) && !ttispair(itail)) {
	    res = KFALSE;
	    break;
	}
    }
    kapply_cc(K, res);
}

/* Helpers for reduce */

/* NOTE: This is used from both do_reduce_cycle and reduce */
void do_reduce(klisp_State *K, TValue *xparams, TValue obj);

void do_reduce_prec(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: first-pair
    ** xparams[1]: (old-obj . rem-ls)
    ** xparams[2]: cpairs
    ** xparams[3]: prec
    ** xparams[4]: denv
    */ 

    TValue first_pair = xparams[0];
    TValue last_pair = xparams[1];
    TValue ls = kcdr(last_pair);
    int32_t cpairs = ivalue(xparams[2]);
    TValue prec = xparams[3];
    TValue denv = xparams[4];

    /* save the last result of precycle */
    kset_car(last_pair, obj);

    if (cpairs == 0) {
	/* pass the first element to the do_reduce_inc continuation */
	kapply_cc(K, kcar(first_pair));
    } else {
	TValue expr = kcons(K, kunwrap(prec), kcons(K, kcar(ls), KNIL));
	TValue new_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce_prec,
			   5, first_pair, ls, i2tv(cpairs-1), prec, denv);
	kset_cc(K, new_cont);
	ktail_eval(K, expr, denv);
    }
}

void do_reduce_postc(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: postc
    ** xparams[1]: denv
    */
    TValue postc = xparams[0];
    TValue denv = xparams[1];

    TValue expr = kcons(K, kunwrap(postc), kcons(K, obj, KNIL));
    ktail_eval(K, expr, denv);
}

/* This could be avoided by contructing a list and calling
   do_reduce, but the order would be backwards if the cycle
   is processed after the acyclic part */
void do_reduce_combine(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: acyclic result
    ** xparams[1]: bin
    ** xparams[2]: denv
    */

    TValue acyclic_res = xparams[0];
    TValue bin = xparams[1];
    TValue denv = xparams[2];

    /* obj: cyclic_res */
    TValue cyclic_res = obj;
    TValue params = kcons(K, acyclic_res, kcons(K, cyclic_res, KNIL));
    TValue expr = kcons(K, kunwrap(bin), params);
    ktail_eval(K, expr, denv);
}

void do_reduce_cycle(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: first-cpair
    ** xparams[1]: cpairs
    ** xparams[2]: acyclic binary applicative
    ** xparams[3]: prec applicative
    ** xparams[4]: inc applicative
    ** xparams[5]: postc applicative
    ** xparams[6]: denv
    ** xparams[7]: has-acyclic-part?
    */ 

    TValue ls = xparams[0];
    int32_t cpairs = ivalue(xparams[1]);
    TValue bin = xparams[2];
    TValue prec = xparams[3];
    TValue inc = xparams[4];
    TValue postc = xparams[5];
    TValue denv = xparams[6];
    bool has_acyclic_partp = bvalue(xparams[7]);

    /* 
    ** Schedule actions in reverse order 
    */

    if (has_acyclic_partp) {
	TValue acyclic_obj = obj;
	TValue combine_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce_combine,
			       3, acyclic_obj, bin, denv);
	kset_cc(K, combine_cont);
    } /* if there is no acyclic part, just let the result pass through */

    TValue post_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce_postc,
			   2, postc, denv);
    kset_cc(K, post_cont);
    
    /* pass one less so that pre_cont can pass the first argument
       to the continuation */
    TValue in_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce,
			   4, kcdr(ls), i2tv(cpairs - 1), inc, denv);
    kset_cc(K, in_cont);

    /* add dummy to allow passing inert to pre_cont */
    TValue dummy = kcons(K, KINERT, ls);
    /* pass ls as the first pair to be passed to the do_reduce
       continuation */
    TValue pre_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce_prec,
			   5, ls, dummy, i2tv(cpairs), prec, denv);
    kset_cc(K, pre_cont);
    /* this will overwrite dummy, but that's ok */
    kapply_cc(K, KINERT);
}

/* NOTE: This is used from both do_reduce_cycle and reduce */
void do_reduce(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: remaining list
    ** xparams[1]: remaining pairs
    ** xparams[2]: binary applicative (either bin or inc)
    ** xparams[3]: denv
    */ 
    
    TValue ls = xparams[0];
    int32_t pairs = ivalue(xparams[1]);
    TValue bin = xparams[2];
    TValue denv = xparams[3];

    if (pairs == 0) {
	/* NOTE: this continuation could have been avoided (made a
	   tail context) but since it isn't a requirement having
	   this will help with error signaling and backtraces */
	kapply_cc(K, obj);
    } else {
	/* GC: root intermediate objs */
	TValue next = kcar(ls);
	TValue params = kcons(K, obj, kcons(K, next, KNIL));
	TValue expr = kcons(K, kunwrap(bin), params);

	TValue new_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce, 4, 
			       kcdr(ls), i2tv(pairs-1), bin, denv);
	kset_cc(K, new_cont);
	/* use the dynamic environment of the call to reduce */
	ktail_eval(K, expr, denv);
    }
}

/* 6.3.10 reduce */
void reduce(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    
    bind_al3tp(K, "reduce", ptree, "any", anytype, ls, "applicative",
	       ttisapplicative, bin, "any", anytype, id, rest);

    TValue prec, inc, postc;
    bool extended_form = !ttisnil(rest);

    if (extended_form) {
	/* the variables are an artifact of the way bind_3tp macro works,
	 XXX: this will also send wrong error msgs (bad number of arg) */
	bind_3tp(K, "reduce (extended)", rest, 
		 "applicative", ttisapplicative, prec_h, 
		 "applicative", ttisapplicative, inc_h, 
		 "applicative", ttisapplicative, postc_h);
	prec = prec_h;
	inc = inc_h;
	postc = postc_h;
    } else {
	/* dummy init */
	prec = inc = postc = KINERT;
    }

    /* the easy case first */
    if (ttisnil(ls)) {
	kapply_cc(K, id);
    } 

    /* TODO all of these in one procedure */
    int32_t cpairs;
    int32_t pairs = check_list(K, "reduce", true, ls, &cpairs);
    int32_t apairs = pairs - cpairs;
    /* force copy to be able to do all precycles and replace
       the corresponding objs in ls */
    ls = check_copy_list(K, "reduce", ls, true);
    TValue first_cycle_pair = ls;
    int32_t dapairs = apairs;
    /* REFACTOR: add an extra return value to check_copy_list to output
       the last pair of the list */
    while(dapairs--)
	first_cycle_pair = kcdr(first_cycle_pair);

    TValue res;

    if (cpairs != 0) {
	if (!extended_form) {
	    klispE_throw(K, "reduce: no cyclic handling applicatives");
	    return;
	}
	/* make cycle reducing cont */
	TValue cyc_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce_cycle, 8, 
			       first_cycle_pair, i2tv(cpairs), bin, prec, 
			       inc, postc, denv, b2tv(apairs != 0));
	kset_cc(K, cyc_cont);
    }

    if (apairs == 0) {
	/* this will be ignore by cyc_cont */
	res = KINERT;
    } else {
	/* this will pass the parent continuation either
	   a list of (rem-ls result) if there is a cycle or
	   result if there is no cycle, this should be a list
	   and not a regular pair to allow the above case of 
	   a one element list to signal no acyclic part */
	TValue acyc_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_reduce, 4, 
			       kcdr(ls), i2tv(apairs-1), bin, denv);
	kset_cc(K, acyc_cont);
	res = kcar(ls);
    }
    kapply_cc(K, res);
}
