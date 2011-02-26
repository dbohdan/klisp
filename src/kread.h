/*
** kread.h
** Reader for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef kread_h
#define kread_h

#include "kobject.h"
#include "kstate.h"

/*
** Reader interface
*/
TValue kread(klisp_State *K);

#endif

