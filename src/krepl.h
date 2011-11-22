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
void do_repl_exit(klisp_State *K);
void do_repl_read(klisp_State *K);
void do_repl_eval(klisp_State *K);
void do_repl_loop(klisp_State *K);
void do_repl_error(klisp_State *K);

#endif
