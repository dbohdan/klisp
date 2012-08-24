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
    klisp_lock(K);
    Applicative *new_app = klispM_new(K, Applicative);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_app, K_TAPPLICATIVE, 
                K_FLAG_CAN_HAVE_NAME);

    /* applicative specific fields */
    new_app->underlying = underlying;
    klisp_unlock(K);
    return gc2app(new_app);
}
