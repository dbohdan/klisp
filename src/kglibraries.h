/*
** kglibraries.h
** Libraries features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kglibraries_h
#define kglibraries_h

#include "kstate.h"

/* init ground */
void kinit_libraries_ground_env(klisp_State *K);
/* init continuation names */
void kinit_libraries_cont_names(klisp_State *K);

#endif
