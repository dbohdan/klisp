/*
** kgenvironments.c
** Environments features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kground.h"
#include "kpair.h"
#include "kstring.h"
#include "kenvironment.h"
#include "kcontinuation.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgenvironments.h"

/* 4.8.1 environment? */
/* uses typep */

/* 4.8.2 ignore? */
/* uses typep */

/* 4.8.3 eval */
void eval(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    bind_2tp(K, "eval", ptree, "any", anytype, expr,
	     "environment", ttisenvironment, env);

    ktail_eval(K, expr, env);
}

/* 4.8.4 make-environment */
void make_environment(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    TValue new_env;
    if (ttisnil(ptree)) {
	new_env = kmake_empty_environment(K);
	kapply_cc(K, new_env);
    } else if (ttispair(ptree) && ttisnil(kcdr(ptree))) {
	/* special common case of one parent, don't keep a list */
	TValue parent = kcar(ptree);
	if (ttisenvironment(parent)) {
	    new_env = kmake_environment(K, parent);
	    kapply_cc(K, new_env);
	} else {
	    klispE_throw(K, "make-environment: not an environment in "
			 "parent list");
	    return;
	}
    } else {
	/* this is the general case, copy the list but without the
	   cycle if there is any */
	TValue parents = check_copy_env_list(K, "make-environment", ptree);
	new_env = kmake_environment(K, parents);
	kapply_cc(K, new_env);
    }
}

/* 5.10.1 $let */
/* TODO */
