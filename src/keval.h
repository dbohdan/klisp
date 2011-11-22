/*
** keval.h
** klisp eval function
** See Copyright Notice in klisp.h
*/

#ifndef keval_h
#define keval_h

#include "klisp.h"
#include "kstate.h"
#include "kobject.h"

void keval_ofn(klisp_State *K, TValue *xparams, TValue obj, TValue env);
void do_eval_ls(klisp_State *K);
void do_combine(klisp_State *K);

#endif
