/*
** kcondvar.h
** Kernel Libraries
** See Copyright Notice in klisp.h
*/

#ifndef kcondvar_h
#define kcondvar_h

#include "kobject.h"
#include "kstate.h"

TValue kmake_condvar(klisp_State *K, TValue mutex);
void klispV_free(klisp_State *K, Condvar *condvar);

/* LOCK: these functions require that the calling code has 
   acquired the GIL exactly once previous to the call and
   they may temporarily release it to avoid deadlocks */
/* LOCK: underlying mutex should be acquired by this thread */
/* GC: condvar should be rooted */
void kcondvar_wait(klisp_State *K, TValue condvar);
void kcondvar_signal(klisp_State *K, TValue condvar, bool broadcast);

#define kcondvar_mutex(c_) (tv2condvar(c_)->mutex)
#define kcondvar_cond(m_) (tv2condvar(m_)->cond)

#endif
