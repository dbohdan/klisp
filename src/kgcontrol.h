/*
** kgcontrol.h
** Control features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcontrol_h
#define kgcontrol_h

#include "kstate.h"

/* init ground */
void kinit_control_ground_env(klisp_State *K);
/* init continuation names */
void kinit_control_cont_names(klisp_State *K);

#endif
