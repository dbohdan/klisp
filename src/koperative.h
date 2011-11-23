/*
** koperative.h
** Kernel Operatives
** See Copyright Notice in klisp.h
*/

#ifndef koperative_h
#define koperative_h

#include "kobject.h"
#include "kstate.h"

/* TODO: make some specialized constructors for 0, 1 and 2 parameters */

/* GC: Assumes all argps are rooted */
TValue kmake_operative(klisp_State *K, klisp_CFunction fn, int32_t xcount, 
		       ...);

#endif
