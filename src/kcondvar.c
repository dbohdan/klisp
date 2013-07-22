/*
** kcondvar.c
** Kernel Libraries
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kmutex.h"
#include "kcondvar.h"
#include "kmem.h"
#include "kgc.h"
#include "kerror.h"

/* GC: Assumes mutex is rooted */
TValue kmake_condvar(klisp_State *K, TValue mutex) 
{
    Condvar *new_condvar = klispM_new(K, Condvar);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_condvar, K_TCONDVAR, 0);

    /* condvar specific fields */
    new_condvar->mutex = mutex;

    /* XXX no attrs for now */
    int32_t res = pthread_cond_init(&new_condvar->cond, NULL);

    if (res != 0) {
        klispE_throw_simple_with_irritants(K, "Can't create conndition "
                                           "variable", 1, i2tv(res));
        return KNIL;
    }
    return gc2condvar(new_condvar);
}

/* LOCK: GIL should be acquired exactly once */
/* LOCK: underlying mutex should be acquired by this thread */
/* GC: condvar should be rooted */
void kcondvar_wait(klisp_State *K, TValue condvar)
{
    TValue thread = gc2th(K);
    TValue mutex = kcondvar_mutex(condvar);

    if (!tv_equal(thread, kmutex_owner(mutex))) {
        klispE_throw_simple(K, "Can't wait without holding the mutex");
        return;
    }

    /* save mutex info to recover after awakening */
    uint32_t count = kmutex_count(mutex);
    kmutex_owner(mutex) = KMUTEX_NO_OWNER;
    kmutex_count(mutex) = 0;
    
    /* we need to release GIL to avoid deadlocks */
    klisp_unlock(K);
    int res = pthread_cond_wait(&kcondvar_cond(condvar), 
                                &kmutex_mutex(mutex));
    klisp_lock(K);

    /* recover the saved mutex info */
    klisp_assert(!kmutex_is_owned(mutex));

    kmutex_owner(mutex) = thread;
    kmutex_count(mutex) = count;

    /* This shouldn't happen, according to the spec */
    if (res != 0) {
        klispE_throw_simple_with_irritants(K, "Couldn't wait on condvar",
                                           1, i2tv(res));
        return;
    }
}

/* LOCK: GIL should be acquired exactly once */
/* LOCK: underlying mutex should be acquired by this thread */
/* GC: condvar should be rooted */
void kcondvar_signal(klisp_State *K, TValue condvar, bool broadcast)
{
    TValue thread = gc2th(K);
    TValue mutex = kcondvar_mutex(condvar);

    if (!tv_equal(thread, kmutex_owner(mutex))) {
        klispE_throw_simple(K, broadcast? 
                            "Can't broadcast without holding the mutex" :
                            "Can't signal without holding the mutex");
        return;
    }

    int res = broadcast? pthread_cond_broadcast(&kcondvar_cond(condvar)) :
        pthread_cond_signal(&kcondvar_cond(condvar));

    /* This shouldn't happen, according to the spec */
    if (res != 0) {
        klispE_throw_simple_with_irritants(K, broadcast? 
                                           "Couldn't broadcast on condvar" :
                                           "Couldn't signal on condvar",
                                           1, i2tv(res));
        return;
    }
}

void klispV_free(klisp_State *K, Condvar *m)
{
    UNUSED(pthread_cond_destroy(&m->cond));
    klispM_free(K, m);
}
