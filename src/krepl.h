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

void kinit_repl(klisp_State *K);

/* continuation functions */
void exit_fn(klisp_State *K, TValue *xparams, TValue obj);
void read_fn(klisp_State *K, TValue *xparams, TValue obj);
void eval_cfn(klisp_State *K, TValue *xparams, TValue obj);
void loop_fn(klisp_State *K, TValue *xparams, TValue obj);
void error_fn(klisp_State *K, TValue *xparams, TValue obj);

#endif
