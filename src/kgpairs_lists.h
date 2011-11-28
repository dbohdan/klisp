/*
** kgpairs_lists.h
** Pairs and lists features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgpairs_lists_h
#define kgpairs_lists_h

#include "kstate.h"
    
/* init ground */
void kinit_pairs_lists_ground_env(klisp_State *K);
/* init continuation names */
void kinit_pairs_lists_cont_names(klisp_State *K);

#endif
