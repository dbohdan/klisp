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
void do_sv_access(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: static key 
    */
    check_0p(K, ptree);

    TValue key = xparams[0];
    /* this may throw an exception if not bound */
    TValue val = kget_keyed_static_var(K, denv, key);
    kapply_cc(K, val);
}

/* binder returned */
void do_sv_bind(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: static key 
    */
    bind_2tp(K, ptree, "any", anytype, obj,
	         "environment", ttisenvironment, env);
    UNUSED(denv); 
    TValue key = xparams[0];
    /* GC: all objs are rooted in ptree, or xparams */
    TValue new_env = kmake_keyed_static_env(K, env, key, obj);
    kapply_cc(K, new_env);
}

/* 11.1.1 make-static-dynamic-variable */
void make_keyed_static_variable(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv); 
    UNUSED(xparams);

    check_0p(K, ptree);
    /* the key is just a dummy pair */
    TValue key = kcons(K, KINERT, KINERT);
    krooted_tvs_push(K, key);
    TValue a = kmake_applicative(K, do_sv_access, 1, key);
    krooted_tvs_push(K, a);
    TValue b = kmake_applicative(K, do_sv_bind, 1, key);
    krooted_tvs_push(K, b);
    TValue ls = klist(K, 2, b, a);

    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    kapply_cc(K, ls);
}


/* init ground */
void kinit_kgks_vars_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 11.1.1 make-keyed-static-variable */
    add_applicative(K, ground_env, "make-keyed-static-variable", 
                    make_keyed_static_variable, 0); 
}
