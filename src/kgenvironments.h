/*
** kgenvironments.h
** Environments features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgpairs_lists_h
#define kgpairs_lists_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.8.1 environment? */
/* uses typep */

/* 4.8.2 ignore? */
/* uses typep */

/* 4.8.3 eval */
void eval(klisp_State *K, TValue *xparams, TValue ptree, 
	  TValue denv);

/* 4.8.4 make-environment */
void make_environment(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* 5.10.1 $let */
/* TODO */

#endif
