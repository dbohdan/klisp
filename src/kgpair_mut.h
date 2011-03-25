/*
** kgpair_mut.h
** Pair mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgpairs_mut_h
#define kgpairs_mut_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* Helper (also used by $vau, $lambda, etc) */
TValue copy_es_immutable_h(klisp_State *K, char *name, TValue ptree, 
			   bool mut_flag);

/* 4.7.1 set-car!, set-cdr! */
void set_carB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

void set_cdrB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper for copy-es & copy-es-immutable */
void copy_es(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 4.7.2 copy-es-immutable */
/* uses copy_es helper */


/* 5.8.1 encycle! */
void encycleB(klisp_State *K, TValue *xparams, TValue ptree, 
	      TValue denv);

/* 6.4.1 append! */
/* TODO */

/* 6.4.2 copy-es */
/* uses copy_es helper */

/* 6.4.3 assq */
/* TODO */

/* 6.4.3 memq? */
void memqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

#endif
