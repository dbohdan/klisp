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
TValue kread_from_port(klisp_State *K, TValue port, bool mut);
TValue kread_peek_char_from_port(klisp_State *K, TValue port, bool peek);

#endif

