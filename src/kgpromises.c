/*
** kgencapsulations.c
** Encapsulations features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpromise.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgpromises.h"

/* 9.1.1 promise? */
/* uses typep */

/* 9.1.2 force */
/* TODO */

/* 9.1.3 $lazy */
/* TODO */

/* 9.1.4 memoize */
void memoize(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1p(K, "memoize", ptree, exp);
    TValue new_prom = kmake_promise(K, KNIL, KNIL, exp, KNIL);
    kapply_cc(K, new_prom);
}
