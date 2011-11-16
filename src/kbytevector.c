/*
** kbytevector.c
** Kernel Byte Vectors
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "kbytevector.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* Constructors */
TValue kbytevector_new_g(klisp_State *K, bool m, uint32_t size)
{
    Bytevector *new_bytevector;

    /* XXX: find a better way to do this! */
    if (size == 0 && ttisbytevector(K->empty_bytevector)) {
	return K->empty_bytevector;
    }

    new_bytevector = klispM_malloc(K, sizeof(Bytevector) + size);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_bytevector, K_TBYTEVECTOR, m? 0 : K_FLAG_IMMUTABLE);

    /* bytevector specific fields */
    new_bytevector->mark = KFALSE;
    new_bytevector->size = size;

    /* clear the buffer */
    memset(new_bytevector->b, 0, size);

    return gc2bytevector(new_bytevector);
}

TValue kbytevector_new(klisp_State *K, uint32_t size)
{
    return kbytevector_new_g(K, true, size);
}

TValue kbytevector_new_imm(klisp_State *K, uint32_t size)
{
    return kbytevector_new_g(K, false, size);
}

/* both obj1 and obj2 should be bytevectors */
bool kbytevector_equalp(TValue obj1, TValue obj2)
{
    klisp_assert(ttisbytevector(obj1) && ttisbytevector(obj2));

    Bytevector *bytevector1 = tv2bytevector(obj1);
    Bytevector *bytevector2 = tv2bytevector(obj2);

    if (bytevector1->size == bytevector2->size) {
	return (bytevector1->size == 0) ||
	    (memcmp(bytevector1->b, bytevector2->b, bytevector1->size) == 0);
    } else {
	return false;
    }
}

bool kbytevectorp(TValue obj) { return ttisbytevector(obj); }
