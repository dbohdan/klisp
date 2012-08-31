/*
** klimits.h
** Limits, basic types, and some other `installation-dependent' definitions
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is from lua (greatly reduced)
*/

#ifndef klimits_h
#define klimits_h

#include <limits.h>
#include <stddef.h>

#ifdef KUSE_ASSERTS
#include <assert.h>
#define klisp_assert(c) (assert(c))
#endif
#include "klisp.h"

/* internal assertions for in-house debugging */
#ifdef klisp_assert

#define check_exp(c,e)		(klisp_assert(c), (e))

#else

#define klisp_assert(c)		((void)(c))
#define check_exp(c,e)		(e)

#endif


#ifndef UNUSED
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif

#ifndef cast
#define cast(t, exp)	((t)(exp))
#endif

/*
** conversion of pointer to integer
** this is for hashing only; there is no problem if the integer
** cannot hold the whole pointer value
*/
#define IntPoint(p)  ((uint32_t)(p))

/* minimum size for the string table (must be power of 2) */
#ifndef MINSTRTABSIZE
#define MINSTRTABSIZE	32
#endif

/* minimum size for the name & cont_name tables (must be power of 2) */
#ifndef MINNAMETABSIZE
#define MINNAMETABSIZE	32
#endif

#ifndef MINCONTNAMETABSIZE
#define MINCONTNAMETABSIZE	32
#endif

#ifndef MINTHREADTABSIZE
#define MINTHREADTABSIZE	32
#endif

/* minimum size for the require table (must be power of 2) */
#ifndef MINREQUIRETABSIZE
#define MINREQUIRETABSIZE	32
#endif

/* starting size for ground environment hashtable */
/* at last count, there were about 200 bindings in ground env */
#define ENVTABSIZE	512

/* starting size for string port buffers */
#ifndef MINSTRINGPORTBUFFER
#define MINSTRINGPORTBUFFER	256
#endif

/* starting size for bytevector port buffers */
#ifndef MINBYTEVECTORPORTBUFFER
#define MINBYTEVECTORPORTBUFFER	256
#endif

/* starting size for readline buffer */
#ifndef MINREADLINEBUFFER
#define MINREADLINEBUFFER	80
#endif

/* XXX for now ignore the return values */
#ifndef klisp_lock
#include <pthread.h>
#define klisp_lock(K) ({                                    \
            if (K->gil_count == 0) {                        \
                K->gil_count = 1;                           \
                UNUSED(pthread_mutex_lock(&G(K)->gil));     \
            } else {                                        \
                ++K->gil_count;                             \
            }})

#define klisp_unlock(K) ({                                  \
            if (K->gil_count <= 1) {                        \
                K->gil_count = 0;                           \
                UNUSED(pthread_mutex_unlock(&G(K)->gil));   \
            } else {                                        \
                --K->gil_count;                             \
            }})

/* this will work no matter how many times  (even 0) the lock was acquired */
#define klisp_unlock_all(K) ({                          \
            if (K->gil_count > 0) {                     \
                K->gil_count = 1;                       \
                klisp_unlock(K);                        \
            }})

#endif

/* These were the original defines */
#ifndef klisp_lock
#define klisp_lock(K)     ((void) 0) 
#define klisp_unlock(K)   ((void) 0)
#endif

#ifndef klispi_threadyield
#define klispi_threadyield(K)     {klisp_unlock(K); klisp_lock(K);}
#endif

#endif
