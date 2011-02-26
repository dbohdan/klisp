/*
** kwrite.h
** Writer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef kwrite_h
#define kwrite_h

#include "kobject.h"
#include "kstate.h"

/*
** Writer interface
*/
void kwrite(klisp_State *K, TValue obj);
void knewline(klisp_State *K);

#endif

