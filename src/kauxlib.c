/*
** kauxlib.h
** Auxiliary functions for klisp
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is from lua, but is greatly reduced (for now)
*/

#include <stddef.h>
#include <stdlib.h>

#include "klisp.h"
#include "kstate.h"

/* generic alloc function */
static void *k_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void)ud;
  (void)osize;

  if (nsize == 0) {
    free(ptr);
    return NULL;
  } else {
    return realloc(ptr, nsize);
  }
}

/*
** Create a new state with the default allocator
*/
klisp_State *klispL_newstate (void)
{
  klisp_State *K = klisp_newstate(k_alloc, NULL);
  /* TODO: set any panic functions or something like that... */
  return K;
}

