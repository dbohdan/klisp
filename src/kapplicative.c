/*
** kapplicative.c
** Kernel Applicatives
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kapplicative.h"
#include "kmem.h"
#include "kgc.h"

TValue kwrap(klisp_State *K, TValue underlying)
{
    return kmake_applicative(K, KNIL, KNIL, underlying);
}

TValue kmake_applicative(klisp_State *K, TValue name, TValue si, 
			 TValue underlying)
{
    krooted_tvs_push(K, name);
    krooted_tvs_push(K, si);
    krooted_tvs_push(K, underlying);
    Applicative *new_app = klispM_new(K, Applicative);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_app, K_TAPPLICATIVE, 0);

    /* applicative specific fields */
    new_app->name = name;
    new_app->si = si;
    new_app->underlying = underlying;
    return gc2app(new_app);
}
