/*
** kencapsulation.c
** Kernel Encapsulation Types
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kmem.h"
#include "kstate.h"
#include "kencapsulation.h"
#include "kpair.h"
#include "kgc.h"

bool kis_encapsulation_type(TValue enc, TValue key)
{
    return ttisencapsulation(enc) && tv_equal(kget_enc_key(enc), key);
}

/* GC: Assumes that key & val are rooted */
TValue kmake_encapsulation(klisp_State *K, TValue key, TValue val)
{
    klisp_lock(K);
    Encapsulation *new_enc = klispM_new(K, Encapsulation);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_enc, K_TENCAPSULATION, 0);

    /* encapsulation specific fields */
    new_enc->key = key;
    new_enc->value = val;

    klisp_unlock(K);
    return gc2enc(new_enc);
}

TValue kmake_encapsulation_key(klisp_State *K)
{
    return kcons(K, KINERT, KINERT);
}
