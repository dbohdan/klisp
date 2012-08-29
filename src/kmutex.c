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

/* GC: Assumes env & ext_list are roooted */
/* ext_list should be immutable (and it may be empty) */
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

void klispX_free(klisp_State *K, Mutex *m)
{
/* XXX/??? Is it okay if the mutex wasn't correctly created?, 
   i.e. the contructor throwed an error*/
    UNUSED(pthread_mutex_destroy(&m->mutex));
    klispM_free(K, m);
}
