/*
** krepl.h
** klisp repl
** See Copyright Notice in klisp.h
*/

#ifndef krepl_h
#define krepl_h

#include "klisp.h"
#include "kstate.h"
#include "kobject.h"

void loop_fn(klisp_State *K, TValue *xparams, TValue obj);
void eval_cfn(klisp_State *K, TValue *xparams, TValue obj);
void exit_fn(klisp_State *K, TValue *xparams, TValue obj);

#endif
