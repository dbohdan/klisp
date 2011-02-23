/*
** kauxlib.h
** Auxiliary functions for klisp
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is from lua, but is greatly reduced (for now)
*/

#ifndef kauxlib_h
#define kauxlib_h


#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "klisp.h"

/*
** Create a new state with the default allocator
*/
klisp_State *klispL_newstate (void);

#endif
