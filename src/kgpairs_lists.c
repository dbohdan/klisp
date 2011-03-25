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
/* TODO */

/* 6.3.5 filter */
/* TODO */

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
