/*
** kencapsulation.h
** Kernel Encapsulation Types
** See Copyright Notice in klisp.h
*/

#ifndef kencapsulation_h
#define kencapsulation_h

#include "kobject.h"
#include "kstate.h"

/* GC: Assumes that key & val are rooted */
TValue kmake_encapsulation(klisp_State *K, TValue key, TValue val);

TValue kmake_encapsulation_key(klisp_State *K);
bool kis_encapsulation_type(TValue enc, TValue key);

/* LOCK: these are immutable, so they don't need locking */
#define kget_enc_val(e_)(tv2enc(e_)->value)
#define kget_enc_key(e_)(tv2enc(e_)->key)

#endif
