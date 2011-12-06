/*
** kvector.c
** Kernel Vectors (heterogenous arrays)
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "kvector.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* helper function allocating vectors */

/* XXX I'm not too convinced this is the best way to handle the empty
   vector... Try to find a better way */
static Vector *kvector_alloc(klisp_State *K, bool m, uint32_t length)
{
    Vector *new_vector;

    if (length > (SIZE_MAX - sizeof(Vector)) / sizeof(TValue))
        klispM_toobig(K);

    klisp_assert(!m || length > 0);

    size_t size = sizeof(Vector) + length * sizeof(TValue);
    new_vector = (Vector *) klispM_malloc(K, size);
    klispC_link(K, (GCObject *) new_vector, K_TVECTOR,
                (m? 0 : K_FLAG_IMMUTABLE));
    new_vector->mark = KFALSE;
    new_vector->sizearray = length;

    return new_vector;
}

TValue kvector_new_sf(klisp_State *K, uint32_t length, TValue fill)
{
    Vector *v = kvector_alloc(K, true, length);
    for (int i = 0; i < length; i++)
        v->array[i] = fill;
    return gc2vector(v);
}

TValue kvector_new_bs_g(klisp_State *K, bool m,
                        const TValue *buf, uint32_t length)
{
    Vector *v = kvector_alloc(K, m, length);
    memcpy(v->array, buf, sizeof(TValue) * length);
    return gc2vector(v);
}

bool kvectorp(TValue obj)
{
    return ttisvector(obj);
}

bool kimmutable_vectorp(TValue obj)
{
    return ttisvector(obj) && kis_immutable(obj); 
}

bool kmutable_vectorp(TValue obj)
{
    return ttisvector(obj) && kis_mutable(obj);
}
