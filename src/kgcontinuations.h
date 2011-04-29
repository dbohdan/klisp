/*
** kgcontinuations.h
** Continuations features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcontinuations_h
#define kgcontinuations_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* Helpers (also used in keyed dynamic code) */
void do_pass_value(klisp_State *K, TValue *xparams, TValue obj);

/* 7.1.1 continuation? */
/* uses typep */

/* 7.2.2 call/cc */
void call_cc(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 7.2.3 extend-continuation */
void extend_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
			 TValue denv);

/* 7.2.4 guard-continuation */
void guard_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv);

/* 7.2.5 continuation->applicative */
void continuation_applicative(klisp_State *K, TValue *xparams, TValue ptree, 
			      TValue denv);

/* 7.2.6 root-continuation */
/* done in kground.c/krepl.c */

/* 7.2.7 error-continuation */
/* done in kground.c/krepl.c */

/* 7.3.1 apply-continuation */
void apply_continuation(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv);

/* 7.3.2 $let/cc */
void Slet_cc(klisp_State *K, TValue *xparams, TValue ptree, 
	     TValue denv);

/* 7.3.3 guard-dynamic-extent */
void guard_dynamic_extent(klisp_State *K, TValue *xparams, TValue ptree, 
			  TValue denv);

/* 7.3.4 exit */    
void kgexit(klisp_State *K, TValue *xparams, TValue ptree, 
	    TValue denv);

void do_extended_cont(klisp_State *K, TValue *xparams, TValue obj);
void do_pass_value(klisp_State *K, TValue *xparams, TValue obj);

#endif
