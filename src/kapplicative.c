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

/* GC: Assumes underlying is rooted */
TValue kwrap(klisp_State *K, TValue underlying)
{
    Applicative *new_app = klispM_new(K, Applicative);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_app, K_TAPPLICATIVE, 0);

    /* applicative specific fields */
    new_app->underlying = underlying;
    return gc2app(new_app);
}
