/*
** kgcombiners.h
** Combiners features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcombiners_h
#define kgcombiners_h

#include "kstate.h"

/* init ground */
void kinit_combiners_ground_env(klisp_State *K);
/* init continuation names */
void kinit_combiners_cont_names(klisp_State *K);

#endif
