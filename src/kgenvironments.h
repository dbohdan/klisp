/*
** kgenvironments.h
** Environments features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgenvironments_h
#define kgenvironments_h

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

/* 6.7.1 $binds? */
/* TODO */

/* 6.7.2 get-current-environment */
void get_current_environment(klisp_State *K, TValue *xparams, TValue ptree, 
			     TValue denv);

/* 6.7.3 make-kernel-standard-environment */
/* TODO */

/* 6.7.4 $let* */
/* TODO */

/* 6.7.5 $letrec */
/* TODO */

/* 6.7.6 $letrec* */
/* TODO */

/* 6.7.7 $let-redirect */
/* TODO */

/* 6.7.8 $let-safe */
/* TODO */

/* 6.7.9 $remote-eval */
/* TODO */

/* 6.7.10 $bindings->environment */
/* TODO */

#endif
