/*
** kmem.c
** Interface to Memory Manager
** See Copyright Notice in klisp.h
*/


/*
** SOURCE NOTE: This is from Lua, but greatly shortened
*/

#include <stddef.h>
#include <stdio.h>
#include <assert.h>

#include "klisp.h"
#include "kstate.h"
#include "klimits.h"
#include "kmem.h"
#include "kerror.h"
#include "kgc.h"

#define MINSIZEARRAY	4

/*
** About the realloc function:
** void * frealloc (void *ud, void *ptr, size_t osize, size_t nsize);
** (`osize' is the old size, `nsize' is the new size)
**
** klisp ensures that (ptr == NULL) iff (osize == 0).
**
** * frealloc(ud, NULL, 0, x) creates a new block of size `x'
**
** * frealloc(ud, p, x, 0) frees the block `p'
** (in this specific case, frealloc must return NULL).
** particularly, frealloc(ud, NULL, 0, 0) does nothing
** (which is equivalent to free(NULL) in ANSI C)
**
** frealloc returns NULL if it cannot create or reallocate the area
** (any reallocation to an equal or smaller size cannot fail!)
*/

void *klispM_growaux_ (klisp_State *K, void *block, int *size, size_t size_elems,
                       int32_t limit, const char *errormsg) {
    void *newblock;
    int32_t newsize;
    if (*size >= limit/2) {  /* cannot double it? */
        if (*size >= limit)  /* cannot grow even a little? */
            klispE_throw_simple(K, (char *) errormsg); /* XXX */
        newsize = limit;  /* still have at least one free place */
    }
    else {
        newsize = (*size)*2;
        if (newsize < MINSIZEARRAY)
            newsize = MINSIZEARRAY;  /* minimum size */
    }
    newblock = klispM_reallocv(K, block, *size, newsize, size_elems);
    *size = newsize;  /* update only when everything else is OK */
    return newblock;
}


void *klispM_toobig (klisp_State *K) {
    /* TODO better msg */
    klispE_throw_simple(K, "(mem) block too big");
    return NULL;  /* to avoid warnings */
}


/*
** generic allocation routine.
*/
void *klispM_realloc_ (klisp_State *K, void *block, size_t osize, size_t nsize) {
    klisp_assert((osize == 0) == (block == NULL));

    /* TEMP: for now only Stop the world GC */
    /* TEMP: prevent recursive call of klispC_fullgc() */
#ifdef KUSE_GC
    if (nsize > 0 && K->totalbytes - osize + nsize >= K->GCthreshold) {
#ifdef KDEBUG_GC
        printf("GC START, total_bytes: %d\n", K->totalbytes);
#endif
        klispC_fullgc(K);
#ifdef KDEBUG_GC
        printf("GC END, total_bytes: %d\n", K->totalbytes);
#endif
    }
#endif

    block = (*K->frealloc)(K->ud, block, osize, nsize);

    if (block == NULL && nsize > 0) {
        /* TEMP: try GC if there is no more mem */
        /* TODO: make this a catchable error */
        fprintf(stderr, MEMERRMSG);
        abort();
    }
    klisp_assert((nsize == 0) == (block == NULL));
    K->totalbytes = (K->totalbytes - osize) + nsize;
    return block;
}
