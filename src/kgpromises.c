/*
** kgpromises.c
** Promises features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpromise.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgpromises.h"

/* Continuations */
void do_handle_result(klisp_State *K);


/* SOURCE_NOTE: this is mostly an adaptation of the library derivation
   in the report */

/* 9.1.1 promise? */
/* uses typep */

/* Helper for force */
void do_handle_result(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: promise
    */
    TValue prom = xparams[0];

    /* check to see if promise was determined before the eval completed */
    if (ttisnil(kpromise_maybe_env(prom))) {
        /* discard obj, return previous result */
        kapply_cc(K, kpromise_exp(prom));
    } else if (ttispromise(obj)) {
        /* force iteratively, by sharing pairs so that when obj
           determines a value, prom also does */
        TValue node = kpromise_node(obj);
        kpromise_node(prom) = node;
        TValue expr = kpromise_exp(prom);
        TValue maybe_env = kpromise_maybe_env(prom);
        if (ttisnil(maybe_env)) {
            /* promise was already determined */
            kapply_cc(K, expr);
        } else {
            TValue new_cont = kmake_continuation(K, kget_cc(K),
                                                 do_handle_result, 1, prom);
            kset_cc(K, new_cont);
            ktail_eval(K, expr, maybe_env);
        }
    } else {
        /* memoize result */
        TValue node = kpromise_node(prom);
        kset_car(node, obj);
        kset_cdr(node, KNIL);
    }
}

/* 9.1.2 force */
void force(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1p(K, ptree, obj);
    if (!ttispromise(obj)) {
        /* non promises force to themselves */
        kapply_cc(K, obj);
    } else if (ttisnil(kpromise_maybe_env(obj))) {
        /* promise was already determined */
        kapply_cc(K, kpromise_exp(obj));
    } else {
        TValue expr = kpromise_exp(obj);
        TValue env = kpromise_maybe_env(obj);
        TValue new_cont = kmake_continuation(K, kget_cc(K), do_handle_result, 
                                             1, obj);
        kset_cc(K, new_cont);
        ktail_eval(K, expr, env);
    }
}

/* 9.1.3 $lazy */
void Slazy(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);

    bind_1p(K, ptree, exp);
    TValue new_prom = kmake_promise(K, exp, denv);
    kapply_cc(K, new_prom);
}

/* 9.1.4 memoize */
/* in kghelpers.c */

/* $delay it's actually a short hand for ($lazy (memoize ...)) */
void Sdelay(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1p(K, ptree, exp);
    TValue promise_body = kcons(K, exp, KNIL);
    krooted_vars_push(K, &promise_body);
    promise_body = kcons(K, G(K)->memoize_app, promise_body);
    TValue new_prom = kmake_promise(K, promise_body, denv);
    krooted_vars_pop(K);
    kapply_cc(K, new_prom);
}

/* init ground */
void kinit_promises_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /* 9.1.1 promise? */
    add_applicative(K, ground_env, "promise?", typep, 2, symbol, 
                    i2tv(K_TPROMISE));
    /* 9.1.2 force */
    add_applicative(K, ground_env, "force", force, 0); 
    /* 9.1.3 $lazy */
    add_operative(K, ground_env, "$lazy", Slazy, 0); 
    /* 9.1.4 memoize */
    add_applicative(K, ground_env, "memoize", memoize, 0); 
    /* 9.1.5? $delay */
    add_applicative(K, ground_env, "$delay", Sdelay, 0); 
}

/* XXX lock? */
/* init continuation names */
void kinit_promises_cont_names(klisp_State *K)
{
    Table *t = tv2table(G(K)->cont_name_table);

    add_cont_name(K, t, do_handle_result, "promise-handle-result");
}
