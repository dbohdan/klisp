/*
** kblob.c
** Kernel Blobs (byte vectors)
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "kblob.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* Constructors */
TValue kblob_new_g(klisp_State *K, bool m, uint32_t size)
{
    Blob *new_blob;

    /* XXX: find a better way to do this! */
    if (size == 0 && ttisblob(K->empty_blob)) {
	return K->empty_blob;
    }

    new_blob = klispM_malloc(K, sizeof(Blob) + size);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_blob, K_TBLOB, m? 0 : K_FLAG_IMMUTABLE);

    /* blob specific fields */
    new_blob->mark = KFALSE;
    new_blob->size = size;

    /* clear the buffer */
    memset(new_blob->b, 0, size);

    return gc2blob(new_blob);
}

TValue kblob_new(klisp_State *K, uint32_t size)
{
    return kblob_new_g(K, true, size);
}

TValue kblob_new_imm(klisp_State *K, uint32_t size)
{
    return kblob_new_g(K, false, size);
}

/* both obj1 and obj2 should be blobs */
bool kblob_equalp(TValue obj1, TValue obj2)
{
    klisp_assert(ttisblob(obj1) && ttisblob(obj2));

    Blob *blob1 = tv2blob(obj1);
    Blob *blob2 = tv2blob(obj2);

    if (blob1->size == blob2->size) {
	return (blob1->size == 0) ||
	    (memcmp(blob1->b, blob2->b, blob1->size) == 0);
    } else {
	return false;
    }
}

bool kblobp(TValue obj) { return ttisblob(obj); }
