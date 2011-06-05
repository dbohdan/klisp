/*
** kgks_vars.h
** Keyed Static Variables features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgks_vars_h
#define kgks_vars_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 11.1.1 make-static-dynamic-variable */
void make_keyed_static_variable(klisp_State *K, TValue *xparams, 
				 TValue ptree, TValue denv);

/* init ground */
void kinit_kgks_vars_ground_env(klisp_State *K);

#endif
