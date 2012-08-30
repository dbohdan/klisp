/*
** kmutex.c
** Kernel Libraries
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kmutex.h"
#include "kmem.h"
#include "kgc.h"
#include "kerror.h"

TValue kmake_mutex(klisp_State *K) 
{
    Mutex *new_mutex = klispM_new(K, Mutex);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_mutex, K_TMUTEX, 0);

    /* mutex specific fields */
    new_mutex->count = 0;
    new_mutex->owner = KMUTEX_NO_OWNER; /* no owner */

    /* XXX no attrs for now */
    int32_t res = pthread_mutex_init(&new_mutex->mutex, NULL);

    if (res != 0) {
        klispE_throw_simple_with_irritants(K, "Can't create mutex", 1, 
                                           i2tv(res));
        return KNIL;
    }
    return gc2mutex(new_mutex);
}

/* LOCK: GIL should be acquired exactly once */
void kmutex_lock(klisp_State *K, TValue mutex)
{
    TValue thread = gc2th(K);
    if (tv_equal(thread, kmutex_owner(mutex))) {
        if (kmutex_count(mutex) == KMUTEX_MAX_COUNT) {
            klispE_throw_simple(K, "Mutex count overflow");
            return;
        }
        ++kmutex_count(mutex);
    } else {
        /* we need to release GIL to avoid deadlocks */
        klisp_unlock(K);
        int res = pthread_mutex_lock(&kmutex_mutex(mutex));
        klisp_lock(K);

        if (res != 0) {
            klispE_throw_simple_with_irritants(K, "Can't lock mutex",
                                               1, i2tv(res));
            return;
        }

        klisp_assert(!kmutex_is_owned(mutex));
        kmutex_owner(mutex) = thread;
        kmutex_count(mutex) = 1;
    }
}

/* LOCK: GIL should be acquired exactly once */
void kmutex_unlock(klisp_State *K, TValue mutex)
{
    TValue thread = gc2th(K);
    if (!kmutex_is_owned(mutex)) {
        klispE_throw_simple(K, "The mutex isn't locked");
        return;
    } else if (tv_equal(thread, kmutex_owner(mutex))) {
        if (kmutex_count(mutex) == 1) {
            int res = pthread_mutex_unlock(&kmutex_mutex(mutex));

            if (res != 0) {
                klispE_throw_simple_with_irritants(K, "Can't unlock mutex",
                                                   1, i2tv(res));
                return;
            }

            kmutex_owner(mutex) = KMUTEX_NO_OWNER;
            kmutex_count(mutex) = 0;
        } else {
            --kmutex_count(mutex);
        }
    } else {
        klispE_throw_simple(K, "The mutex is locked by a different thread");
        return;
    }
}

/* LOCK: GIL should be acquired exactly once */
bool kmutex_trylock(klisp_State *K, TValue mutex)
{
    TValue thread = gc2th(K);
    if (tv_equal(thread, kmutex_owner(mutex))) {
        kmutex_lock(K, mutex); /* this will check max_count */
        return true;
    } else if (kmutex_is_owned(mutex)) {
        return false;
    } else {
        /* we need to release GIL to avoid deadlocks */
        klisp_unlock(K);
        int res = pthread_mutex_trylock(&kmutex_mutex(mutex));
        klisp_lock(K);

        if (res == 0) {
            klisp_assert(!kmutex_is_owned(mutex));
            kmutex_owner(mutex) = thread;
            kmutex_count(mutex) = 1;
            return true;
        } else if (res == EBUSY) {
            return false;
        } else {
            klispE_throw_simple_with_irritants(K, "Error on trylock mutex",
                                               1, i2tv(res));
            return false;
        }
    } 
}

void klispX_free(klisp_State *K, Mutex *m)
{
/* XXX/??? Is it okay if the mutex wasn't correctly created?, 
   i.e. the contructor throwed an error*/
    UNUSED(pthread_mutex_destroy(&m->mutex));
    klispM_free(K, m);
}
