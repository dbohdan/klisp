/*
** kpromise.c
** Kernel Promises
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kpromise.h"
#include "kpair.h"
#include "kmem.h"
#include "kgc.h"

TValue kmake_promise(klisp_State *K, TValue name, TValue si,
		     TValue exp, TValue maybe_env)
{
    krooted_tvs_push(K, name);
    krooted_tvs_push(K, si);
    krooted_tvs_push(K, exp);
    krooted_tvs_push(K, maybe_env);
    Promise *new_prom = klispM_new(K, Promise);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_prom, K_TPROMISE, 0);

    /* promise specific fields */
    new_prom->name = name;
    new_prom->si = si;
    new_prom->node = KNIL; /* temp in case of GC */
    krooted_tvs_push(K, gc2prom(new_prom));
    new_prom->node = kcons(K, exp, maybe_env);
    krooted_tvs_pop(K);
    return gc2prom(new_prom);
}
