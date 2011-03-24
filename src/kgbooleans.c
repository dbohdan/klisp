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
