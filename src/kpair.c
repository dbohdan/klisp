/*
** kpair.c
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

/* XXX: for malloc */
#include <stdlib.h>
/* TODO: use a generalized alloc function */

#include "kpair.h"
#include "kobject.h"

/* TODO: Out of memory errors */
/* TEMP: for now all pairs are mutable */
TValue kcons(TValue car, TValue cdr) 
{
    Pair *new_pair = malloc(sizeof(Pair));

    new_pair->next = NULL;
    new_pair->gct = 0;
    new_pair->tt = K_TPAIR;
    new_pair->car = car;
    new_pair->cdr = cdr;

    return gc2pair(new_pair);
}
