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

TValue kmake_promise(klisp_State *K, TValue name, TValue si,
		     TValue exp, TValue maybe_env)
{
    Promise *new_prom = klispM_new(K, Promise);

    /* header + gc_fields */
    new_prom->next = K->root_gc;
    K->root_gc = (GCObject *)new_prom;
    new_prom->gct = 0;
    new_prom->tt = K_TPROMISE;
    new_prom->flags = 0;

    /* promise specific fields */
    new_prom->name = name;
    new_prom->si = si;
    /* GC: root new_prom before cons */
    new_prom->node = kcons(K, exp, maybe_env);
    return gc2prom(new_prom);
}
