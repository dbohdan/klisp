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
void eval_ls_cfn(klisp_State *K, TValue *xparams, TValue obj);
void combine_cfn(klisp_State *K, TValue *xparams, TValue obj);

#endif
