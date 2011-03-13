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
/* TEMP: for now it takes only two argument */
void eqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_2p(K, "eq?", ptree, obj1, obj2);

    bool res = eq2p(K, obj1, obj2);
    kapply_cc(K, b2tv(res));
}
