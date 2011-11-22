/*
** kgcontrol.h
** Control features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcontrol_h
#define kgcontrol_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.5.1 inert? */
/* uses typep */

/* 4.5.2 $if */

void Sif(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.1.1 $sequence */
void Ssequence(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helpers for $cond */
TValue split_check_cond_clauses(klisp_State *K, TValue clauses, 
				TValue *bodies);


/* 5.6.1 $cond */
void Scond(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.9.1 for-each */
void for_each(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

void do_seq(klisp_State *K);
void do_cond(klisp_State *K);
void do_select_clause(klisp_State *K);
void do_for_each(klisp_State *K);

/* init ground */
void kinit_control_ground_env(klisp_State *K);

#endif
