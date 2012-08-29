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

/* LOCK: these functions require that the calling code has 
   acquired the GIL exactly once previous to the call and
   they may temporarily release it to avoid deadlocks */
void kmutex_lock(klisp_State *K, TValue mutex);
void kmutex_unlock(klisp_State *K, TValue mutex);
bool kmutex_trylock(klisp_State *K, TValue mutex);

#define kmutex_is_owned(m_) (ttisthread(tv2mutex(m_)->owner))
#define kmutex_owner(m_) (tv2mutex(m_)->owner)
#define kmutex_mutex(m_) (tv2mutex(m_)->mutex)
#define kmutex_count(m_) (tv2mutex(m_)->count)

// #define KMUTEX_MAX_COUNT UINT32_MAX
#define KMUTEX_MAX_COUNT 255

#endif
