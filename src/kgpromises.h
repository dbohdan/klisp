/*
** kgpromises.h
** Promises features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgpromises_h
#define kgpromises_h

#include "kstate.h"

/* init ground */
void kinit_promises_ground_env(klisp_State *K);
/* init continuation names */
void kinit_promises_cont_names(klisp_State *K);

#endif
