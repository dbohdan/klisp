/*
** kgmodules.h
** Modules features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgmodules_h
#define kgmodules_h

#include "kstate.h"

/* init ground */
void kinit_modules_ground_env(klisp_State *K);
/* init continuation names */
void kinit_modules_cont_names(klisp_State *K);

#endif
