/*
** kgstrings.c
** Strings features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"

#include "kghelpers.h"

/* ?.1? thread? */
/* uses typep */

/* ?.2? get-current-thread */
static void get_current_thread(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    check_0p(K, ptree);
    kapply_cc(K, gc2th(K));
}

/* init ground */
void kinit_threads_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from a mix of scheme implementations and the pthreads library
    */

    /* ?.1? thread? */
    add_applicative(K, ground_env, "thread?", typep, 2, symbol, 
                    i2tv(K_TTHREAD));

    /* ?.2? get-current-thread */
    add_applicative(K, ground_env, "get-current-thread", get_current_thread, 0);
}
