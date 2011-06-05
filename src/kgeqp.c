/*
** kgeqp.c
** Equivalence under mutation features for the ground environment
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
#include "kerror.h"

#include "kghelpers.h"
#include "kgeqp.h"

/* 4.2.1 eq? */
/* 6.5.1 eq? */
/* NOTE: this does 2 passes but could do it in one */
void eqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    int32_t pairs = check_list(K, "eq?", true, ptree, NULL);

    /* In this case we can get away without comparing the
       first and last element on a cycle because eq? is
       symetric, (cf: ftyped_bpred) */
    int32_t comps = pairs - 1;
    TValue tail = ptree;
    TValue res = KTRUE;
    while(comps-- > 0) {  /* comps could be -1 if ptree is nil */
	TValue first = kcar(tail);
	tail = kcdr(tail); /* tail only advances one place per iteration */
	TValue second = kcar(tail);

	if (!eq2p(K, first, second)) {
	    res = KFALSE;
	    break;
	}
    }

    kapply_cc(K, res);
}

/* init ground */
void kinit_eqp_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;
    /* 4.2.1 eq? */
    /* 6.5.1 eq? */
    add_applicative(K, ground_env, "eq?", eqp, 0);
}
