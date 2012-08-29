/*
** kmutex.h
** Kernel Libraries
** See Copyright Notice in klisp.h
*/

#ifndef kmutex_h
#define kmutex_h

#include "kobject.h"
#include "kstate.h"

TValue kmake_mutex(klisp_State *K);
void klispX_free(klisp_State *K, Mutex *mutex);

/* XXX The functions for locking and unlocking are in kgthreads, for now, 
   mainly because they use locking... */

#define kmutex_is_owned(m_) (ttisthread(tv2mutex(p_)->owner))
#define kmutex_get_owner(m_) (tv2mutex(p_)->owner)
#define kmutex_get_mutex(m_) (tv2mutex(p_)->mutex)
#define kmutex_get_count(m_) (tv2mutex(p_)->count)

#endif
