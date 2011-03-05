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
TValue kmake_operative(klisp_State *K, TValue name, TValue si, 
		       klisp_Ofunc fn, int xcount, ...);

#endif
