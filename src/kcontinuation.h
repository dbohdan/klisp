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
TValue kmake_continuation(klisp_State *K, TValue parent, klisp_CFunction fn, 
                          int xcount, ...);

/* Interceptions */
void cont_app(klisp_State *K);
TValue create_interception_list(klisp_State *K, TValue src_cont, 
                                TValue dst_cont);
void do_interception(klisp_State *K);

#endif
