/*
** kgbooleans.h
** Boolean features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kpair.h"
#include "ksymbol.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kghelpers.h"

/* 4.1.1 boolean? */
/* uses typep */

/* 6.1.1 not? */
void notp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, "not?", ptree, "boolean", ttisboolean, tv_b);

    TValue res = kis_true(tv_b)? KFALSE : KTRUE;
    kapply_cc(K, res);
}

/* Helper for type checking booleans */
bool kbooleanp(TValue obj) { return ttisboolean(obj); }

/* 6.1.2 and? */
void andp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    int32_t dummy; /* don't care about cycle pairs */
    int32_t pairs = check_typed_list(K, "and?", "boolean", kbooleanp,
				     true, ptree, &dummy);
    TValue res = KTRUE;
    TValue tail = ptree;
    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	if (kis_false(first)) {
	    res = KFALSE;
	    break;
	}
    }
    kapply_cc(K, res);
}

/* 6.1.3 or? */
void orp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    int32_t dummy; /* don't care about cycle pairs */
    int32_t pairs = check_typed_list(K, "or?", "boolean", kbooleanp,
				     true, ptree, &dummy);
    TValue res = KFALSE;
    TValue tail = ptree;
    while(pairs--) {
	TValue first = kcar(tail);
	tail = kcdr(tail);
	if (kis_true(first)) {
	    res = KTRUE;
	    break;
	}
    }
    kapply_cc(K, res);
}

/* Helpers for $and? & $or? */

/*
** operands is a list, the other cases are handled before calling 
** term-bool is the termination boolean, i.e. the boolean that terminates 
** evaluation early and becomes the result of $and?/$or?
** it is #t for $or? and #f for $and?
** both $and? & $or? have to allow boolean checking while performing a tail 
** call that is acomplished by checking if the current continuation will 
** perform a boolean check, and in that case, no continuation is created
*/
void do_Sandp_Sorp(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: termination boolean
    ** xparams[2]: remaining operands
    ** xparams[3]: denv
    */
    TValue sname = xparams[0];
    TValue term_bool = xparams[1];
    TValue ls = xparams[2];
    TValue denv = xparams[3];

    if (!ttisboolean(obj)) {
	klispE_throw_extra(K, ksymbol_buf(sname), ": expected boolean");
	return;
    } else if (ttisnil(ls) || tv_equal(obj, term_bool)) {
	/* in both cases the value to be returned is obj:
	   if there are no more operands it is obvious otherwise, if
	   the termination bool is found:
	   $and? returns #f when it finds #f and $or? returns #t when it 
	   finds #t */
	kapply_cc(K, obj);
    } else {
	TValue first = kcar(ls);
	ls = kcdr(ls);
	/* This is the important part of tail context + bool check */
	if (!ttisnil(ls) || !kis_bool_check_cont(kget_cc(K))) {
	    TValue new_cont = 
		kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_Sandp_Sorp, 
				   4, sname, term_bool, ls, denv);
	    /* 
	    ** Mark as a bool checking cont this is needed in the last operand
	    ** to allow both tail recursive behaviour and boolean checking.
	    ** While it is not necessary if this is not the last operand it
	    ** avoids a continuation in the last evaluation of the inner form 
	    ** in the common use of 
	    ** ($and?/$or? ($or?/$and? ...) ...)
	    */
	    kset_bool_check_cont(new_cont);
	    kset_cc(K, new_cont);
	}
	ktail_eval(K, first, denv);
    }
}

void Sandp_Sorp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: termination boolean
    */
    TValue sname = xparams[0];
    TValue term_bool = xparams[1];
    
    TValue ls = check_copy_list(K, ksymbol_buf(sname), ptree, false);
    /* This will work even if ls is empty */
    TValue new_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_Sandp_Sorp, 
			   4, sname, term_bool, ls, denv);
    /* there's no need to mark it as bool checking, no evaluation
       is done in the dynamic extent of this cont */
    kset_cc(K, new_cont);
    kapply_cc(K, knegp(term_bool)); /* pass dummy value to start */
}

/* 6.1.4 $and? */
/* uses Sandp_Sorp */

/* 6.1.5 $or? */
/* uses Sandp_Sorp */
