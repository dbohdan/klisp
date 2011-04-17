/*
** kcontinuation.h
** Kernel Continuations
** See Copyright Notice in klisp.h
*/

#ifndef kcontinuation_h
#define kcontinuation_h

#include "kobject.h"
#include "kstate.h"

/* TODO: make some specialized constructors for 0, 1 and 2 parameters */
TValue kmake_continuation(klisp_State *K, TValue parent, klisp_Cfunc fn, 
			  int xcount, ...);

#endif
