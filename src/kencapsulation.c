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

TValue kmake_encapsulation(klisp_State *K, TValue name, TValue si,
			   TValue key, TValue val)
{
    Encapsulation *new_enc = klispM_new(K, Encapsulation);

    /* header + gc_fields */
    new_enc->next = K->root_gc;
    K->root_gc = (GCObject *)new_enc;
    new_enc->gct = 0;
    new_enc->tt = K_TENCAPSULATION;
    new_enc->flags = 0;

    /* encapsulation specific fields */
    new_enc->name = name;
    new_enc->si = si;
    new_enc->key = key;
    new_enc->value = val;

    return gc2enc(new_enc);
}

TValue kmake_encapsulation_key(klisp_State *K)
{
    return kcons(K, KINERT, KINERT);
}
