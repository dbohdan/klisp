/*
** kgkd_vars.c
** Keyed Dynamic Variables features for the ground environment
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
#include "kcontinuation.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kenvironment.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgkd_vars.h"

/*
** A dynamic key is a pair with a boolean in the car indicating if the
** variable is bound and an arbitrary object in the cdr representing the
** currently bound value.
*/

/* Helpers for make-keyed-dynamic-variable */
/* in kghelpers */


/* 10.1.1 make-keyed-dynamic-variable */
void make_keyed_dynamic_variable(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv); 
    UNUSED(xparams);

    check_0p(K, ptree);
    TValue key = kcons(K, KFALSE, KINERT);
    krooted_tvs_push(K, key);
    TValue a = kmake_applicative(K, do_access, 1, key);
    krooted_tvs_push(K, a);
    TValue b = kmake_applicative(K, do_bind, 1, key);
    krooted_tvs_push(K, b);
    TValue ls = klist(K, 2, b, a);

    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    kapply_cc(K, ls);
}

/* init ground */
void kinit_kgkd_vars_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /* 10.1.1 make-keyed-dynamic-variable */
    add_applicative(K, ground_env, "make-keyed-dynamic-variable", 
                    make_keyed_dynamic_variable, 0); 
}
