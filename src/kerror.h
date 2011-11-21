/*
** kerror.h
** Simple error notification and handling (TEMP)
** See Copyright Notice in klisp.h
*/


#ifndef kerror_h
#define kerror_h

#include <stdbool.h>
#include <errno.h>

#include "klisp.h"
#include "kstate.h"
#include "kpair.h" /* for klist */

TValue klispE_new(klisp_State *K, TValue who, TValue cont, TValue msg, 
		  TValue irritants);

void klispE_free(klisp_State *K, Error *error);

void klispE_throw_simple(klisp_State *K, char *msg);
void klispE_throw_with_irritants(klisp_State *K, char *msg, TValue irritants);

void klispE_throw_system_error_with_irritants(klisp_State *K, const char *service, int errnum, TValue irritants);

/* evaluates K__ more than once */
/* the objects should be rooted */
#define klispE_throw_simple_with_irritants(K__, msg__, ...)		\
    { TValue ls__ = klist(K__, __VA_ARGS__);				\
    krooted_tvs_push(K__, ls__);					\
    /* the pop is implicit in throw_with_irritants */			\
    klispE_throw_with_irritants(K__, msg__, ls__); }

/* the objects should be rooted */
#define klispE_throw_errno_with_irritants(K__, service__, ...) \
    { \
        int errnum__ = errno; \
        TValue ls__ = klist(K__, __VA_ARGS__); \
        krooted_tvs_push(K__, ls__); \
        klispE_throw_system_error_with_irritants(K__, service__, errnum__, ls__); \
    }

#define klispE_throw_errno_simple(K__, service__) \
    klispE_throw_system_error_with_irritants(K__, service__, errno, KNIL);

TValue klispE_describe_errno(klisp_State *K, const char *service, int errnum);

#endif
