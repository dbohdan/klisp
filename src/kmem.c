/*
** kmem.c
** Interface to Memory Manager
** See Copyright Notice in klisp.h
*/


/*
** SOURCE NOTE: This is from Lua, but greatly shortened
*/

#include <stddef.h>
#include <assert.h>

#include "klisp.h"
#include "klimits.h"
#include "kmem.h"
#include "kerror.h"

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


/*
** generic allocation routine.
*/
void *klispM_realloc_ (klisp_State *K, void *block, size_t osize, size_t nsize) {
  klisp_assert((osize == 0) == (block == NULL));

  block = (*K->frealloc)(K->ud, block, osize, nsize);
  if (block == NULL && nsize > 0)
      klispE_throw(K, MEMERRMSG, false);
  klisp_assert((nsize == 0) == (block == NULL));
  K->totalbytes = (K->totalbytes - osize) + nsize;
  return block;
}
