/*
** kgenvironments.h
** Environments features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgenvironments_h
#define kgenvironments_h

#include "kstate.h"

/* init ground */
void kinit_environments_ground_env(klisp_State *K);
/* init continuation names */
void kinit_environments_cont_names(klisp_State *K);

#endif
