/*
** kpair.c
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

#include <stdarg.h>

#include "kpair.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* GC: assumes car & cdr are rooted */
TValue kcons_g(klisp_State *K, bool m, TValue car, TValue cdr) 
{
    Pair *new_pair = klispM_new(K, Pair);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_pair, K_TPAIR, (m? 0 : K_FLAG_IMMUTABLE));

    /* pair specific fields */
    new_pair->mark = KFALSE;
    new_pair->car = car;
    new_pair->cdr = cdr;

    return gc2pair(new_pair);
}

#define MAX_LIST_N 16

/* GC: assumes all argps are rooted */
TValue klist_g(klisp_State *K, bool m, int32_t n, ...)
{
    va_list argp;

    klisp_assert(n < MAX_LIST_N);

    /* don't use any of the klisp dummys, because this is 
       called from many places */
    TValue dummy = kcons_g(K, m, KINERT, KNIL);
    krooted_tvs_push(K, dummy);
    TValue tail = dummy;

    va_start(argp, n);
    for (int i = 0; i < n; i++) {
	TValue next_car = va_arg(argp, TValue);
	TValue np = kcons_g(K, m, next_car, KNIL); 
	kset_cdr_unsafe(K, tail, np);
	tail = np;
    }
    va_end(argp);

    krooted_tvs_pop(K);
    return kcdr(dummy);
}


bool kpairp(TValue obj) { return ttispair(obj); }
