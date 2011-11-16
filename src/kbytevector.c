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

/* General constructor for bytevectors */
TValue kbytevector_new_bs_g(klisp_State *K, bool m, const uint8_t *buf, 
			uint32_t size)
{
    return m? kbytevector_new_bs(K, buf, size) :
	kbytevector_new_bs_imm(K, buf, size);
}

/* 
** Constructors for immutable bytevectors
*/

/* main constructor for immutable bytevectors */
TValue kbytevector_new_bs_imm(klisp_State *K, const uint8_t *buf, uint32_t size)
{
    /* Does it make sense to put them in the string table 
       (i.e. interning them)?, we have two different constructors just in case */

    /* XXX: find a better way to do this! */
    if (size == 0 && ttisbytevector(K->empty_bytevector)) {
	return K->empty_bytevector;
    }

    Bytevector *new_bb;

    if (size > (SIZE_MAX - sizeof(Bytevector)))
	klispM_toobig(K);

    new_bb = (Bytevector *) klispM_malloc(K, sizeof(Bytevector) + size);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_bb, K_TBYTEVECTOR, K_FLAG_IMMUTABLE);

    /* bytevector specific fields */
    new_bb->mark = KFALSE;
    new_bb->size = size;

    if (size != 0) {
	memcpy(new_bb->b, buf, size);
    }
    
    return gc2bytevector(new_bb);
}

/* 
** Constructors for mutable bytevectors
*/

/* main constructor for mutable bytevectors */
/* with just size */
TValue kbytevector_new_s(klisp_State *K, uint32_t size)
{
    Bytevector *new_bb;

    if (size == 0) {
	klisp_assert(ttisbytevector(K->empty_bytevector));
	return K->empty_bytevector;
    }

    new_bb = klispM_malloc(K, sizeof(Bytevector) + size);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_bb, K_TBYTEVECTOR, 0);

    /* bytevector specific fields */
    new_bb->mark = KFALSE;
    new_bb->size = size;

    /* the buffer is initialized elsewhere */

    return gc2bytevector(new_bb);
}

/* with buffer & size */
TValue kbytevector_new_bs(klisp_State *K, const uint8_t *buf, uint32_t size)
{
    if (size == 0) {
	klisp_assert(ttisbytevector(K->empty_bytevector));
	return K->empty_bytevector;
    }

    TValue new_bb = kbytevector_new_s(K, size);
    memcpy(kbytevector_buf(new_bb), buf, size);
    return new_bb;
}

/* with size and fill uint8_t */
TValue kbytevector_new_sf(klisp_State *K, uint32_t size, uint8_t fill)
{
    if (size == 0) {
	klisp_assert(ttisbytevector(K->empty_bytevector));
	return K->empty_bytevector;
    }

    TValue new_bb = kbytevector_new_s(K, size);
    memset(kbytevector_buf(new_bb), fill, size);
    return new_bb;
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
