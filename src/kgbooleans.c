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

/* 6.1.4 $and? */
/* TODO */

/* 6.1.5 $or? */
/* TODO */
