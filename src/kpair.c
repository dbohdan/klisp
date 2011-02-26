/*
** kpair.c
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

#include "kpair.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"

/* TEMP: for now all pairs are mutable */
TValue kcons(klisp_State *K, TValue car, TValue cdr) 
{
    Pair *new_pair = klispM_new(K, Pair);

    new_pair->next = NULL;
    new_pair->gct = 0;
    new_pair->tt = K_TPAIR;
    new_pair->mark = KFALSE;
    new_pair->car = car;
    new_pair->cdr = cdr;

    return gc2pair(new_pair);
}
