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
void klispE_throw(klisp_State *K, char *msg);
/* TEMP: for throwing with extra msg info */
void klispE_throw_extra(klisp_State *K, char *msg, char *extra_msg);

#endif
