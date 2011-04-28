/*
** kerror.h
** Simple error notification and handling (TEMP)
** See Copyright Notice in klisp.h
*/


#ifndef kerror_h
#define kerror_h

#include <stdbool.h>

#include "klisp.h"
#include "kstate.h"

TValue klispE_new(klisp_State *K, TValue who, TValue cont, TValue msg, 
		  TValue irritants);
void klispE_throw_simple(klisp_State *K, char *msg);
void klispE_throw_with_irritants(klisp_State *K, char *msg, TValue irritants);

/* evaluates K__ more than once */
#define klispE_throw_simple_with_irritants(K__, msg__, ...)		\
    { TValue ls__ = klist(K__, __VA_ARGS__);				\
    krooted_tvs_push(K__, ls__);					\
    /* the pop is implicit in throw_with_irritants */			\
    klispE_throw_with_irritants(K__, msg__, ls__); }



#endif
