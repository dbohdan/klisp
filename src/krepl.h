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
/* init continuation names */
void kinit_repl_cont_names(klisp_State *K);

#endif
