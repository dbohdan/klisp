/*
** kgerror.h
** Error handling features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgerrors_h
#define kgerrors_h

#include "kstate.h"

/* init ground */
void kinit_error_ground_env(klisp_State *K);

/* Second stage of itialization of ground environment. Must be
 * called after initializing general error continuation
 * K->error_cont. */
void kinit_error_hierarchy(klisp_State *K);

#endif
