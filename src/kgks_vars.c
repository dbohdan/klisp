/*
** kgks_vars.c
** Keyed Static Variables features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kcontinuation.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kenvironment.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgks_vars.h"

/* Helpers for make-static-dynamic-variable */

/* accesor returned */
void do_sv_access(klisp_State *K, TValue *xparams, TValue ptree, 
	       TValue denv)
{
    /*
    ** xparams[0]: static key 
    */
    check_0p(K, "keyed-static-get", ptree);

    TValue key = xparams[0];
    /* this may throw an exception if not bound */
    TValue val = kget_keyed_static_var(K, denv, key);
    kapply_cc(K, val);
}

/* binder returned */
void do_sv_bind(klisp_State *K, TValue *xparams, TValue ptree, 
		TValue denv)
{
    /*
    ** xparams[0]: static key 
    */
    bind_2tp(K, "keyed-static-bind", ptree, "any", anytype, obj,
	      "environment", ttisenvironment, env);
    UNUSED(denv); 
    TValue key = xparams[0];
    /* GC: root intermediate objs */
    TValue new_env = kmake_keyed_static_env(K, env, key, obj);
    kapply_cc(K, new_env);
}

/* 11.1.1 make-static-dynamic-variable */
void make_keyed_static_variable(klisp_State *K, TValue *xparams, 
				 TValue ptree, TValue denv)
{
    UNUSED(denv); 
    UNUSED(xparams);

    check_0p(K, "make-keyed-static-variable", ptree);
    /* the key is just a dummy pair */
    TValue key = kcons(K, KINERT, KINERT);
    TValue a = kwrap(K, kmake_operative(K, KNIL, KNIL, do_sv_access, 1, key));
    TValue b = kwrap(K, kmake_operative(K, KNIL, KNIL, do_sv_bind, 1, key));
    TValue ls = kcons(K, b, kcons(K, a, KNIL));
    kapply_cc(K, ls);
}