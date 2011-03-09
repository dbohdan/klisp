/*
** kapplicative.c
** Kernel Applicatives
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kapplicative.h"
#include "kmem.h"

TValue kwrap(klisp_State *K, TValue underlying)
{
    return kmake_applicative(K, KNIL, KNIL, underlying);
}

TValue kmake_applicative(klisp_State *K, TValue name, TValue si, 
			 TValue underlying)
{
    Applicative *new_app = klispM_new(K, Applicative);

    /* header + gc_fields */
    new_app->next = K->root_gc;
    K->root_gc = (GCObject *)new_app;
    new_app->gct = 0;
    new_app->tt = K_TAPPLICATIVE;
    new_app->flags = 0;

    /* applicative specific fields */
    new_app->name = name;
    new_app->si = si;
    new_app->underlying = underlying;
    return gc2app(new_app);
}
