/*
** kgenv_mut.h
** Environment mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgenv_mut_h
#define kgenv_mut_h

#include "kstate.h"

/* init ground */
void kinit_env_mut_ground_env(klisp_State *K);
/* init continuation names */
void kinit_env_mut_cont_names(klisp_State *K);


#endif
