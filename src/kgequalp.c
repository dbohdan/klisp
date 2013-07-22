/*
** kgequalp.h
** Equivalence up to mutation features for the ground environment
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
#include "kvector.h" 
#include "kstring.h" /* for kstring_equalp */
#include "kbytevector.h" /* for kbytevector_equalp */
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgequalp.h"

/* 4.3.1 equal? */
/* 6.6.1 equal? */

/*
** equal? is O(n) where n is the number of pairs.
** Based on [1] "A linear algorithm for testing equivalence of finite automata"
** by J.E. Hopcroft and R.M.Karp
** List merging from [2] "A linear list merging algorithm"
** by J.E. Hopcroft and J.D. Ullman
** Idea to look up these papers from srfi 85: 
** "Recursive Equivalence Predicates" by William D. Clinger
*/
void equalp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

    int32_t pairs;
    check_list(K, true, ptree, &pairs, NULL);

    /* In this case we can get away without comparing the
       first and last element on a cycle because equal? is
       symetric, (cf: ftyped_bpred) */
    int32_t comps = pairs - 1;
    TValue tail = ptree;
    TValue res = KTRUE;
    while(comps-- > 0) {  /* comps could be -1 if ptree is nil */
        TValue first = kcar(tail);
        tail = kcdr(tail); /* tail only advances one place per iteration */
        TValue second = kcar(tail);

        if (!equal2p(K, first, second)) {
            res = KFALSE;
            break;
        }
    }

    kapply_cc(K, res);
}

/* init ground */
void kinit_equalp_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;
    /* 4.3.1 equal? */
    /* 6.6.1 equal? */
    add_applicative(K, ground_env, "equal?", equalp, 0);
}
