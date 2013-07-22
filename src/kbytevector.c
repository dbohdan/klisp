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
/* for immutable table */
#include "kstring.h" 

/* Constructors */

/* General constructor for bytevectors */
TValue kbytevector_new_bs_g(klisp_State *K, bool m, const uint8_t *buf, 
                            uint32_t size)
{
    return m? kbytevector_new_bs(K, buf, size) :
        kbytevector_new_bs_imm(K, buf, size);
}

/* LOCK: GIL should be acquired */
static uint32_t get_bytevector_hash(const uint8_t *buf, uint32_t size)
{
    uint32_t h = size; /* seed */
    size_t step = (size>>5)+1; /* if bytevector is too long, don't hash all 
                                  its bytes */
    size_t size1;
    for (size1 = size; size1 >= step; size1 -= step)  /* compute hash */
        h = h ^ ((h<<5)+(h>>2)+ buf[size1-1]);

    return h;
}

/* Looks for a bytevector in the stringtable and returns a pointer
   to it if found or NULL otherwise.  */
static Bytevector *search_in_bb_table(klisp_State *K, const uint8_t *buf, 
                                      uint32_t size, uint32_t h)
{

    for (GCObject *o = G(K)->strt.hash[lmod(h, G(K)->strt.size)];
         o != NULL; o = o->gch.next) {
        klisp_assert(o->gch.tt == K_TKEYWORD || o->gch.tt == K_TSYMBOL || 
                     o->gch.tt == K_TSTRING || o->gch.tt == K_TBYTEVECTOR);
		        
        if (o->gch.tt != K_TBYTEVECTOR) continue;

        Bytevector *tb = (Bytevector *) o;
        if (tb->size == size && (memcmp(buf, tb->b, size) == 0)) {
            /* bytevector may be dead */
            if (isdead(G(K), o)) changewhite(o);
            return tb;
        }
    }
    return NULL;
}


/* 
** Constructors for immutable bytevectors
*/

/* main constructor for immutable bytevectors */
TValue kbytevector_new_bs_imm(klisp_State *K, const uint8_t *buf, uint32_t size)
{
    uint32_t h = get_bytevector_hash(buf, size);

    /* first check to see if it's in the stringtable */
    Bytevector *new_bb = search_in_bb_table(K, buf, size, h);

    if (new_bb != NULL) { /* found */
        return gc2bytevector(new_bb);
    }

    /* If it exits the loop, it means it wasn't found, hash is still in h */
    /* REFACTOR: move all of these to a new function */

    if (size > (SIZE_MAX - sizeof(Bytevector)))
        klispM_toobig(K);

    new_bb = (Bytevector *) klispM_malloc(K, sizeof(Bytevector) + size);

    /* header + gc_fields */
    /* can't use klispC_link, because strings use the next pointer
       differently */
    new_bb->gct = klispC_white(G(K));
    new_bb->tt = K_TBYTEVECTOR;
    new_bb->kflags = K_FLAG_IMMUTABLE;
    new_bb->si = NULL;

    /* bytevector specific fields */
    new_bb->hash = h;
    new_bb->mark = KFALSE;
    new_bb->size = size;

    if (size != 0) {
        memcpy(new_bb->b, buf, size);
    }
    
    /* add to the string/symbol table (and link it) */
    stringtable *tb;
    tb = &G(K)->strt;
    h = lmod(h, tb->size);
    new_bb->next = tb->hash[h];  /* chain new entry */
    tb->hash[h] = (GCObject *)(new_bb);
    tb->nuse++;
    TValue ret_tv = gc2bytevector(new_bb);
    if (tb->nuse > ((uint32_t) tb->size) && tb->size <= INT32_MAX / 2) {
        krooted_tvs_push(K, ret_tv); /* save in case of gc */
        klispS_resize(K, tb->size*2);  /* too crowded */
        krooted_tvs_pop(K);
    }

    return ret_tv;
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
        klisp_assert(ttisbytevector(G(K)->empty_bytevector));
        return G(K)->empty_bytevector;
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
        klisp_assert(ttisbytevector(G(K)->empty_bytevector));
        return G(K)->empty_bytevector;
    }

    TValue new_bb = kbytevector_new_s(K, size);
    memcpy(kbytevector_buf(new_bb), buf, size);
    return new_bb;
}

/* with size and fill uint8_t */
TValue kbytevector_new_sf(klisp_State *K, uint32_t size, uint8_t fill)
{
    if (size == 0) {
        klisp_assert(ttisbytevector(G(K)->empty_bytevector));
        return G(K)->empty_bytevector;
    }

    TValue new_bb = kbytevector_new_s(K, size);
    memset(kbytevector_buf(new_bb), fill, size);
    return new_bb;
}

/* both obj1 and obj2 should be bytevectors */
bool kbytevector_equalp(klisp_State *K, TValue obj1, TValue obj2)
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
bool kimmutable_bytevectorp(TValue obj)
{ 
    return ttisbytevector(obj) && kis_immutable(obj); 
}
bool kmutable_bytevectorp(TValue obj)
{ 
    return ttisbytevector(obj) && kis_mutable(obj); 
}
