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

/* 5.7.1 get-list-metrics */
void get_list_metrics(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_1p(K, "get-list-metrics", ptree, obj);
    int32_t pairs = 0;
    TValue tail = obj;

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

    /* GC: root intermediate pairs */
    TValue res = kcons(K, i2tv(apairs), kcons(K, i2tv(cpairs), KNIL));
    res = kcons(K, i2tv(pairs), kcons(K, i2tv(nils), res));
    kapply_cc(K, res);
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
    /* XXX: should be integer instead of fixint, but that's all
       we have for now */
    bind_2tp(K, "list-tail", ptree, "any", anytype, obj,
	     "finite integer", ttisfixint, tk);
    int k = ivalue(tk);
    if (k < 0) {
	klispE_throw(K, "list-tail: negative index");
	return;
    }

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
    (void) denv;
    (void) xparams;
    /* XXX: should be integer instead of fixint, but that's all
       we have for now */
    bind_2tp(K, "list-ref", ptree, "any", anytype, obj,
	     "finite integer", ttisfixint, tk);
    int k = ivalue(tk);
    if (k < 0) {
	klispE_throw(K, "list-ref: negative index");
	return;
    }

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

/* 6.3.3 append */
/* TODO */

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
    kapply_cc(K, kcdr(xparams[0]));
}

/* For cyclic input list: If the result cycle is non empty, 
   close it and return filtered list */
void do_filter_encycle(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: (dummy . complete-ls)
    ** xparams[1]: last non-cycle pair
    */
    /* obj: ((last-evaled . rem-ls) . last-pair) */
    TValue last_pair = kcdr(obj);
    TValue last_non_cycle_pair = xparams[1];

    if (tv_equal(last_pair, last_non_cycle_pair)) {
	/* no cycle in result, so put the nil at the end.
	   this is necessary because it is now pointing
	   to the first cycle pair */
	kset_cdr(last_non_cycle_pair, KNIL);
    } else {
	/* There are pairs in the cycle, so close it */
	TValue first_cycle_pair = kcdr(last_non_cycle_pair);
	TValue last_cycle_pair = last_pair;
	kset_cdr(last_cycle_pair, first_cycle_pair);
    }

    kapply_cc(K, kcdr(xparams[0]));
}

void do_filter(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: app
    ** xparams[1]: (last-evaled . rem-ls)
    ** xparams[2]: last-pair in result list
    ** xparams[3]: n
    */
    TValue app = xparams[0];
    TValue ls = xparams[1];
    TValue last_pair = xparams[2];
    int32_t n = ivalue(xparams[3]);

    if (!ttisboolean(obj)) {
	klispE_throw(K, "filter: expected boolean result");
	return;
    } 
    
    if (kis_false(obj)) {
	kset_cdr(last_pair, kcdr(ls));
    } else {
	last_pair = ls;
    }

    if (n == 0) {
        /* pass the rest of the list and last pair for cycle handling */
	kapply_cc(K, kcons(K, ls, last_pair)); 
    } else {
	TValue new_n = i2tv(n-1);
/* The car of ls here contains the last evaluated object. So the next
   obj to be tested is actually the cadr of ls. */
	TValue first = kcadr(ls);
	ls = kcdr(ls);
	TValue new_env = kmake_empty_environment(K);
	/* have to unwrap the applicative to extra evaluation of first */
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
    ** xparams[0]: (dummy . complete-ls)
    ** xparams[1]: app
    ** xparams[2]: cpairs
    */ 

    TValue dummy = xparams[0];
    TValue app = xparams[1];
    TValue cpairs = xparams[2];

    /* obj: ((last-acyclic obj . cycle) . last-result-pair) */
    TValue ls = kcar(obj);
    TValue last_pair = kcdr(obj);

    /* this continuation will close the cycle and return the list */
    TValue encycle_cont =
 	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_filter_encycle, 2, 
			   dummy, last_pair);

    /* schedule the filtering of the elements of the cycle */
    TValue new_cont = 
	kmake_continuation(K, encycle_cont, KNIL, KNIL, do_filter, 4, app, 
			   ls, last_pair, cpairs);
    kset_cc(K, new_cont);
    /* this will be like a nop (the cdr of last-pair will be setted, but it
       will get rewritten later), and will continue with do_filter */
    kapply_cc(K, KFALSE);
}

/* 6.3.5 filter */
void filter(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    bind_2tp(K, "filter", ptree, "applicative", ttisapplicative, app,
	     "any", anytype, ls);
    /* copy the list to allow filtering by mutating pairs and
       to avoid changes made by the applicative to alter the 
       structure of ls */
    /* REFACTOR: do this in a single pass */
    int32_t cpairs;
    int32_t pairs = check_list(K, "filter", true, ls, &cpairs);
    ls = check_copy_list(K, "filter", ls);
    /* add dummy pair to allow set-cdr! to filter out any pair */
    ls = kcons(K, KINERT, ls);
    
    TValue ret_cont = (cpairs == 0)?
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_ret_cdr, 1, ls)
	: kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_filter_cycle, 3, 
			     ls, app, i2tv(cpairs));
    TValue new_cont = 
	kmake_continuation(K, ret_cont, KNIL, KNIL, do_filter, 4, app, 
			   ls, ls, i2tv(pairs-cpairs));
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

/* 6.3.10 reduce */
/* TODO */
