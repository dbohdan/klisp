/*
** kmem.h
** Interface to Memory Manager
** See Copyright Notice in klisp.h
*/

#ifndef kmem_h
#define kmem_h

/*
** SOURCE NOTE: This is from Lua, but greatly shortened
*/

#include <stddef.h>

#include "klisp.h"

#define MEMERRMSG	"not enough memory"

#define klispM_freemem(L, b, s)	klispM_realloc_(L, (b), (s), 0)
#define klispM_free(L, b)	klispM_realloc_(L, (b), sizeof(*(b)), 0)

#define klispM_malloc(L,t)	klispM_realloc_(L, NULL, 0, (t))
#define klispM_new(L,t)		cast(t *, klispM_malloc(L, sizeof(t)))

void *klispM_realloc_ (klisp_State *K, void *block, size_t oldsize,
		     size_t size);

#endif
