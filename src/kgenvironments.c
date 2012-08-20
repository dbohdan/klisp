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
#include "kpair.h"
#include "kenvironment.h"
#include "kcontinuation.h"
#include "ksymbol.h"
#include "kerror.h"
#include "kport.h" /* for eval_string */
#include "kread.h" /* for eval_string */

#include "kghelpers.h"
#include "kgenvironments.h"

/* Continuations */
void do_let(klisp_State *K);
void do_let_redirect(klisp_State *K);
void do_bindsp(klisp_State *K);
void do_remote_eval(klisp_State *K);
void do_b_to_env(klisp_State *K);
void do_eval_string(klisp_State *K);

/* 4.8.1 environment? */
/* uses typep */

/* 4.8.2 ignore? */
/* uses typep */

/* 4.8.3 eval */
void eval(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

    bind_2tp(K, ptree, "any", anytype, expr,
             "environment", ttisenvironment, env);
    /* TODO: track source code info */
    ktail_eval(K, expr, env);
}

/* 4.8.4 make-environment */
void make_environment(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

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
            klispE_throw_simple(K, "not an environment in "
                                "parent list");
            return;
        }
    } else {
        /* this is the general case, copy the list but without the
           cycle if there is any */
        TValue parents = check_copy_env_list(K, ptree);
        krooted_tvs_push(K, parents);
        new_env = kmake_environment(K, parents);
        krooted_tvs_pop(K);
        kapply_cc(K, new_env);
    }
}

/* Helpers for all the let family */

/* 
** The split-let-bindings function has two cases:
** the 'lets' with a star ($let* and $letrec) allow repeated symbols
** in different bidings (each binding is a different ptree whereas
** in $let, $letrec, $let-redirect and $let-safe, all the bindings
** are collected in a single ptree).
** In both cases the value returned is a list of cars of bindings and
** exprs is modified to point to a list of cadrs of bindings.
** The ptrees are copied as by copy-es-immutable (as with $vau & $lambda)
** If bindings is not finite (or not a list) an error is signaled.
*/

/* GC: assume bindings is rooted */
TValue split_check_let_bindings(klisp_State *K, TValue bindings, 
                                TValue *exprs, bool starp)
{
    TValue cars = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &cars);
    TValue last_car_pair = cars;
    TValue cadrs = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &cadrs);
    TValue last_cadr_pair = cadrs;

    TValue tail = bindings;

    while(ttispair(tail) && !kis_marked(tail)) {
        kmark(tail);
        TValue first = kcar(tail);
        if (!ttispair(first) || !ttispair(kcdr(first)) ||
            !ttisnil(kcddr(first))) {
            unmark_list(K, bindings);
            klispE_throw_simple(K, "bad structure in bindings");
            return KNIL;
        }
	
        TValue new_car = kcons(K, kcar(first), KNIL);
        kset_cdr(last_car_pair, new_car);
        last_car_pair = new_car;
        TValue new_cadr = kcons(K, kcadr(first), KNIL);
        kset_cdr(last_cadr_pair, new_cadr);
        last_cadr_pair = new_cadr;

        tail = kcdr(tail);
    }

    unmark_list(K, bindings);

    if (!ttispair(tail) && !ttisnil(tail)) {
        klispE_throw_simple(K, "expected list");
        return KNIL;
    } else if(ttispair(tail)) {
        klispE_throw_simple(K, "expected finite list"); 
        return KNIL;
    } else {
        TValue res;
        if (starp) {
            /* all bindings are consider individual ptrees in these 'let's,
               replace each ptree with its copy (after checking of course) */
            tail = kcdr(cars);
            while(!ttisnil(tail)) {
                TValue first = kcar(tail);
                TValue copy = check_copy_ptree(K, first, KIGNORE);
                kset_car(tail, copy);
                tail = kcdr(tail);
            }
            res = kcdr(cars);
        } else {
            /* all bindings are consider one ptree in these 'let's */
            res = check_copy_ptree(K, kcdr(cars), KIGNORE);
        }
        *exprs = kcdr(cadrs);
        krooted_vars_pop(K);
        krooted_vars_pop(K);
        return res;
    }
}

/*
** Continuation function for all the let family
** it expects the result of the last evaluation to be matched to 
** this-ptree
*/
void do_let(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: this ptree
    ** xparams[2]: remaining bindings
    ** xparams[3]: remaining exprs
    ** xparams[4]: match environment
    ** xparams[5]: rec/not rec flag
    ** xparams[6]: body
    */
    TValue sname = xparams[0];
    TValue ptree = xparams[1];
    TValue bindings = xparams[2];
    TValue exprs = xparams[3];
    TValue env = xparams[4];
    bool recp = bvalue(xparams[5]);
    TValue body = xparams[6];
    
    match(K, env, ptree, obj);
    
    if (ttisnil(bindings)) {
        if (ttisnil(body)) {
            kapply_cc(K, KINERT);
        } else {
            /* this is needed because seq continuation doesn't check for 
               nil sequence */
            TValue tail = kcdr(body);
            if (ttispair(tail)) {
                TValue new_cont = kmake_continuation(K, kget_cc(K),
                                                     do_seq, 2, tail, env);
                kset_cc(K, new_cont);
#if KTRACK_SI
                /* put the source info of the list including the element
                   that we are about to evaluate */
                kset_source_info(K, new_cont, ktry_get_si(K, body));
#endif
            } 
            ktail_eval(K, kcar(body), env);
        }
    } else {
        TValue new_env = kmake_environment(K, env);
        krooted_tvs_push(K, new_env);
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                               kcar(bindings), kcdr(bindings), kcdr(exprs), 
                               new_env, b2tv(recp), body);
        krooted_tvs_pop(K);
        kset_cc(K, new_cont);
        ktail_eval(K, kcar(exprs), recp? new_env : env);
    }
}

/* 5.10.1 $let */
/* REFACTOR: reuse code in other members of the $let family */
void Slet(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    */
    TValue sname = xparams[0];
    bind_al1p(K, ptree, bindings, body);

    TValue exprs;
    TValue bptree = split_check_let_bindings(K, bindings, &exprs, false);
    krooted_tvs_push(K, bptree);
    krooted_tvs_push(K, exprs);

    check_list(K, true, body, NULL, NULL);
    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

    TValue new_env = kmake_environment(K, denv);
    krooted_tvs_push(K, new_env);
    TValue new_cont = 
        kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                           bptree, KNIL, KNIL, new_env, b2tv(false), body);
    kset_cc(K, new_cont);

    TValue expr = kcons(K, G(K)->list_app, exprs);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, expr, denv);
}

/* Helper for $binds? */
void do_bindsp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: symbol list (may contain cycles)
    ** xparams[1]: symbol list count
    */
    TValue symbols = xparams[0];
    int32_t count = ivalue(xparams[1]);
    
    if (!ttisenvironment(obj)) {
        klispE_throw_simple(K, "expected environment as first argument");
        return;
    }
    TValue env = obj;
    TValue res = KTRUE;

    while(count--) {
        TValue first = kcar(symbols);
        symbols = kcdr(symbols);

        if (!kbinds(K, env, first)) {
            res = KFALSE;
            break;
        }
    }

    kapply_cc(K, res);
}

/* 6.7.1 $binds? */
void Sbindsp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    bind_al1p(K, ptree, env_expr, symbols);

    /* REFACTOR replace with single function check_copy_typed_list */
    int32_t count;
    check_typed_list(K, ksymbolp, true, symbols, &count, NULL);
    symbols = check_copy_list(K, symbols, false, NULL, NULL);

    krooted_tvs_push(K, symbols);
    TValue new_cont = kmake_continuation(K, kget_cc(K), do_bindsp, 
                                         2, symbols, i2tv(count));
    krooted_tvs_pop(K);
    kset_cc(K, new_cont);
    ktail_eval(K, env_expr, denv);
}

/* 6.7.2 get-current-environment */
void get_current_environment(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    check_0p(K, ptree);
    kapply_cc(K, denv);
}

/* 6.7.3 make-kernel-standard-environment */
void make_kernel_standard_environment(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    check_0p(K, ptree);
    
    /* std environments have hashtable for bindings */
    TValue new_env = kmake_table_environment(K, G(K)->ground_env);
//    TValue new_env = kmake_environment(K, G(K)->ground_env);
    kapply_cc(K, new_env);
}

/* 6.7.4 $let* */
void SletS(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    */
    TValue sname = xparams[0];
    bind_al1p(K, ptree, bindings, body);

    TValue exprs;
    TValue bptree = split_check_let_bindings(K, bindings, &exprs, true);
    krooted_tvs_push(K, exprs);
    krooted_tvs_push(K, bptree);
    check_list(K, true, body, NULL, NULL);
    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

    TValue new_env = kmake_environment(K, denv);
    krooted_tvs_push(K, new_env);

    if (ttisnil(bptree)) {
        /* same as $let */
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                               bptree, KNIL, KNIL, new_env, b2tv(false), body);
        kset_cc(K, new_cont);

        TValue expr = kcons(K, G(K)->list_app, exprs);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        ktail_eval(K, expr, denv);
    } else {
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                               kcar(bptree), kcdr(bptree), kcdr(exprs), 
                               new_env, b2tv(false), body);
        kset_cc(K, new_cont);

        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        ktail_eval(K, kcar(exprs), denv);
    }
}

/* 6.7.5 $letrec */
void Sletrec(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    */
    TValue sname = xparams[0];
    bind_al1p(K, ptree, bindings, body);

    TValue exprs;
    TValue bptree = split_check_let_bindings(K, bindings, &exprs, false);
    krooted_tvs_push(K, exprs);
    krooted_tvs_push(K, bptree);

    check_list(K, true, body, NULL, NULL);
    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

    TValue new_env = kmake_environment(K, denv);
    krooted_tvs_push(K, new_env);

    TValue new_cont = 
        kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                           bptree, KNIL, KNIL, new_env, b2tv(true), body);
    kset_cc(K, new_cont);
    
    TValue expr = kcons(K, G(K)->list_app, exprs);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, expr, new_env);
}

/* 6.7.6 $letrec* */
void SletrecS(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    */
    TValue sname = xparams[0];
    bind_al1p(K, ptree, bindings, body);

    TValue exprs;
    TValue bptree = split_check_let_bindings(K, bindings, &exprs, true);
    krooted_tvs_push(K, exprs);
    krooted_tvs_push(K, bptree);
    check_list(K, true, body, NULL, NULL);
    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

    TValue new_env = kmake_environment(K, denv);
    krooted_tvs_push(K, new_env);

    if (ttisnil(bptree)) {
        /* same as $letrec */
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                               bptree, KNIL, KNIL, new_env, b2tv(true), body);
        kset_cc(K, new_cont);

        TValue expr = kcons(K, G(K)->list_app, exprs);

        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        ktail_eval(K, expr, new_env);
    } else {
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                               kcar(bptree), kcdr(bptree), kcdr(exprs), 
                               new_env, b2tv(true), body);
        kset_cc(K, new_cont);

        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        ktail_eval(K, kcar(exprs), new_env);
    }
}

/* Helper for $let-redirect */
void do_let_redirect(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: ptree
    ** xparams[2]: list expr to be eval'ed
    ** xparams[3]: denv
    ** xparams[4]: body
    */
    TValue sname = xparams[0];
    TValue bptree = xparams[1];
    TValue lexpr = xparams[2];
    TValue denv = xparams[3];
    TValue body = xparams[4];
    
    if (!ttisenvironment(obj)) {
        klispE_throw_simple(K, "expected environment"); 
        return;
    }
    TValue new_env = kmake_environment(K, obj);
    krooted_tvs_push(K, new_env);
    TValue new_cont = 
        kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                           bptree, KNIL, KNIL, new_env, b2tv(false), body);
    kset_cc(K, new_cont);

    krooted_tvs_pop(K);
    ktail_eval(K, lexpr, denv);
}

/* 6.7.7 $let-redirect */
void Slet_redirect(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    */
    TValue sname = xparams[0];
    bind_al2p(K, ptree, env_exp, bindings, body);

    TValue exprs;
    TValue bptree = split_check_let_bindings(K, bindings, &exprs, false);
    krooted_tvs_push(K, exprs);
    krooted_tvs_push(K, bptree);

    check_list(K, true, body, NULL, NULL);
    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

    TValue eexpr = kcons(K, G(K)->list_app, exprs);
    krooted_tvs_push(K, eexpr);

    TValue new_cont = 
        kmake_continuation(K, kget_cc(K), do_let_redirect, 5, sname, 
                           bptree, eexpr, denv, body);
    kset_cc(K, new_cont);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, env_exp, denv);
}

/* 6.7.8 $let-safe */
void Slet_safe(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    */
    TValue sname = xparams[0];
    bind_al1p(K, ptree, bindings, body);

    TValue exprs;
    TValue bptree = split_check_let_bindings(K, bindings, &exprs, false);
    krooted_tvs_push(K, exprs);
    krooted_tvs_push(K, bptree);

    check_list(K, true, body, NULL, NULL);

    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

/* according to the definition of the report it should be a child
   of a child of the ground environment, but since this is a fresh
   environment, the semantics are the same */
    TValue new_env = kmake_environment(K, G(K)->ground_env);
    krooted_tvs_push(K, new_env);
    TValue new_cont = 
        kmake_continuation(K, kget_cc(K), do_let, 7, sname, 
                           bptree, KNIL, KNIL, new_env, b2tv(false), body);
    kset_cc(K, new_cont);

    TValue expr = kcons(K, G(K)->list_app, exprs);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, expr, denv);
}

/* 6.7.9 $remote-eval */
void Sremote_eval(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_2p(K, ptree, obj, env_exp);

    TValue new_cont = kmake_continuation(K, kget_cc(K),
                                         do_remote_eval, 1, obj);
    kset_cc(K, new_cont);

    ktail_eval(K, env_exp, denv);
}

/* Helper for $remote-eval */
void do_remote_eval(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    if (!ttisenvironment(obj)) {
        klispE_throw_simple(K, "bad type from second operand "
                            "evaluation (expected environment)");
        return;
    } else {
        TValue eval_exp = xparams[0];
        ktail_eval(K, eval_exp, obj);
    }
}

/* Helper for $bindings->environment */
void do_b_to_env(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: ptree
    ** xparams[1]: created env
    */
    TValue ptree = xparams[0];
    TValue env = xparams[1];
    
    match(K, env, ptree, obj);
    kapply_cc(K, env);
}

/* 6.7.10 $bindings->environment */
void Sbindings_to_environment(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    TValue exprs;
    TValue bptree = split_check_let_bindings(K, ptree, &exprs, false);
    krooted_tvs_push(K, exprs);
    krooted_tvs_push(K, bptree);

    TValue new_env = kmake_environment(K, KNIL);
    krooted_tvs_push(K, new_env);

    TValue new_cont = kmake_continuation(K, kget_cc(K), 
                                         do_b_to_env, 2, bptree, new_env);
    kset_cc(K, new_cont);
    TValue expr = kcons(K, G(K)->list_app, exprs);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, expr, denv);
}

void do_eval_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: environment
    */
    TValue env = xparams[0];
    ktail_eval(K, obj, env);
}

/* ?.? eval-string */
void eval_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_2tp(K, ptree, "string", ttisstring, str,
             "environment", ttisenvironment, env);
    
    /* create a continuation for better stack traces
       in case of error */
    TValue port = kmake_mport(K, str, false, false);
    krooted_tvs_push(K, port);
    TValue cont = kmake_continuation(K, kget_cc(K), do_eval_string, 1, env);
    kset_cc(K, cont);
    krooted_tvs_pop(K);
    
    TValue obj = kread_from_port(K, port, true); /* read mutable pairs */ 
    if (ttiseof(obj)) {
        klispE_throw_simple_with_irritants(K, "No object found in string", 1,
                                           str);
        return;
    }
    krooted_tvs_push(K, obj);
    TValue second_obj = kread_from_port(K, port, true);
    krooted_tvs_pop(K);
    if (!ttiseof(second_obj)) {
        klispE_throw_simple_with_irritants(K, "More than one object found "
                                           "in string", 1, str);
        return;
    }
    kapply_cc(K, obj);
}

/* init ground */
void kinit_environments_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /* 4.8.1 environment? */
    add_applicative(K, ground_env, "environment?", typep, 2, symbol, 
                    i2tv(K_TENVIRONMENT));
    /* 4.8.2 ignore? */
    add_applicative(K, ground_env, "ignore?", typep, 2, symbol, 
                    i2tv(K_TIGNORE));
    /* 4.8.3 eval */
    add_applicative(K, ground_env, "eval", eval, 0);
    /* 4.8.4 make-environment */
    add_applicative(K, ground_env, "make-environment", make_environment, 0);
    /* 5.10.1 $let */
    add_operative(K, ground_env, "$let", Slet, 1, symbol);
    /* 6.7.1 $binds? */
    add_operative(K, ground_env, "$binds?", Sbindsp, 0);
    /* 6.7.2 get-current-environment */
    add_applicative(K, ground_env, "get-current-environment", 
                    get_current_environment, 0);
    /* 6.7.3 make-kernel-standard-environment */
    add_applicative(K, ground_env, "make-kernel-standard-environment", 
                    make_kernel_standard_environment, 0);
    /* 6.7.4 $let* */
    add_operative(K, ground_env, "$let*", SletS, 1, symbol);
    /* 6.7.5 $letrec */
    add_operative(K, ground_env, "$letrec", Sletrec, 1, symbol);
    /* 6.7.6 $letrec* */
    add_operative(K, ground_env, "$letrec*", SletrecS, 1, symbol);
    /* 6.7.7 $let-redirect */
    add_operative(K, ground_env, "$let-redirect", Slet_redirect, 1, symbol);
    /* 6.7.8 $let-safe */
    add_operative(K, ground_env, "$let-safe", Slet_safe, 1, symbol);
    /* 6.7.9 $remote-eval */
    add_operative(K, ground_env, "$remote-eval", Sremote_eval, 0);
    /* 6.7.10 $bindings->environment */
    add_operative(K, ground_env, "$bindings->environment", 
                  Sbindings_to_environment, 1, symbol);
    /* ?.? eval-string */
    add_applicative(K, ground_env, "eval-string", eval_string, 0);
}

/* XXX lock? */
/* init continuation names */
void kinit_environments_cont_names(klisp_State *K)
{
    Table *t = tv2table(G(K)->cont_name_table);
    
    add_cont_name(K, t, do_let, "eval-let");
    add_cont_name(K, t, do_let_redirect, "eval-let-redirect");
    add_cont_name(K, t, do_bindsp, "eval-$binds?-env");
    add_cont_name(K, t, do_remote_eval, "eval-remote-eval-env");
    add_cont_name(K, t, do_eval_string, "eval-string");
    add_cont_name(K, t, do_b_to_env, "bindings-to-env");
}
