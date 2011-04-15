/*
** kpair.c
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

#include "kpair.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

TValue kcons_g(klisp_State *K, bool m, TValue car, TValue cdr) 
{
    krooted_tvs_push(K, car);
    krooted_tvs_push(K, cdr);
    Pair *new_pair = klispM_new(K, Pair);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_pair, K_TPAIR, (m? 0 : K_FLAG_IMMUTABLE));

    /* pair specific fields */
    new_pair->si = KNIL;
    new_pair->mark = KFALSE;
    new_pair->car = car;
    new_pair->cdr = cdr;

    return gc2pair(new_pair);
}

bool kpairp(TValue obj) { return ttispair(obj); }
