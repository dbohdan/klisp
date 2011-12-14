/*
** kmem.h
** Interface to Memory Manager
** See Copyright Notice in klisp.h
*/

#ifndef kmem_h
#define kmem_h

/*
** SOURCE NOTE: This is from Lua
*/

#include <stddef.h>

#include "klisp.h"

#define MEMERRMSG	"not enough memory"

#define klispM_reallocv(L,b,on,n,e)                                     \
    ((cast(size_t, (n)+1) <= SIZE_MAX/(e)) ?  /* +1 to avoid warnings */ \
     klispM_realloc_(L, (b), (on)*(e), (n)*(e)) :                       \
     klispM_toobig(L))

#define klispM_freemem(K, b, s)	klispM_realloc_(K, (b), (s), 0)
#define klispM_free(K, b)	klispM_realloc_(K, (b), sizeof(*(b)), 0)
#define klispM_freearray(L, b, n, t)   klispM_reallocv(L, (b), n, 0, sizeof(t))

#define klispM_malloc(K,t)	klispM_realloc_(K, NULL, 0, (t))
#define klispM_new(K,t)		cast(t *, klispM_malloc(K, sizeof(t)))
#define klispM_newvector(L,n,t)                             \
    cast(t *, klispM_reallocv(L, NULL, 0, n, sizeof(t)))

#define klispM_growvector(L,v,nelems,size,t,limit,e)                    \
    if ((nelems)+1 > (size))                                            \
        ((v)=cast(t *, klispM_growaux_(L,v,&(size),sizeof(t),limit,e)))

#define klispM_reallocvector(L, v,oldn,n,t)                     \
    ((v)=cast(t *, klispM_reallocv(L, v, oldn, n, sizeof(t))))

void *klispM_realloc_ (klisp_State *K, void *block, size_t oldsize,
                       size_t size);
void *klispM_toobig (klisp_State *K);
void *klispM_growaux_ (klisp_State *K, void *block, int *size,
                       size_t size_elem, int limit,
                       const char *errormsg);

#endif
