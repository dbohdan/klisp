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

/* Helpers for all $let family */
TValue split_check_let_bindings(klisp_State *K, char *name, TValue bindings, 
				TValue *exprs, bool starp);
/* 5.10.1 $let */
void Slet(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.7.1 $binds? */
/* TODO */

/* 6.7.2 get-current-environment */
void get_current_environment(klisp_State *K, TValue *xparams, TValue ptree, 
			     TValue denv);

/* 6.7.3 make-kernel-standard-environment */
void make_kernel_standard_environment(klisp_State *K, TValue *xparams, 
				      TValue ptree, TValue denv);

/* 6.7.4 $let* */
void SletS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.7.5 $letrec */
void Sletrec(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.7.6 $letrec* */
void SletrecS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper for $let-redirect */
void do_let_redirect(klisp_State *K, TValue *xparams, TValue obj);

/* 6.7.7 $let-redirect */
void Slet_redirect(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.7.8 $let-safe */
void Slet_safe(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.7.9 $remote-eval */
void Sremote_eval(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper for $remote-eval */
void do_remote_eval(klisp_State *K, TValue *xparams, TValue obj);

/* Helper for $bindings->environment */
void do_b_to_env(klisp_State *K, TValue *xparams, TValue obj);

/* 6.7.10 $bindings->environment */
void Sbindings_to_environment(klisp_State *K, TValue *xparams, TValue ptree, 
			      TValue denv);

#endif
