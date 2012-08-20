/*
** kgcombiners.c
** Combiners features for the ground environment
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
#include "koperative.h"
#include "kapplicative.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgcombiners.h"

/* continuations */
void do_vau(klisp_State *K);

void do_map(klisp_State *K);
void do_map_ret(klisp_State *K);
void do_map_encycle(klisp_State *K);
void do_map_cycle(klisp_State *K);

void do_array_map_ret(klisp_State *K);

/* 4.10.1 operative? */
/* uses typep */

/* 4.10.2 applicative? */
/* uses typep */

/* 4.10.3 $vau */
/* 5.3.1 $vau */
void Svau(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) xparams;
    bind_al2p(K, ptree, vptree, vpenv, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, vptree, vpenv);
    
    krooted_tvs_push(K, vptree);

    /* the body should be a list */
    check_list(K, true, vbody, NULL, NULL);
    vbody = copy_es_immutable_h(K, vbody, false);

    krooted_tvs_push(K, vbody);
    
    TValue new_op = kmake_operative(K, do_vau, 4, vptree, vpenv, vbody, denv);

#if KTRACK_SI
    /* save as source code info the info from the expression whose evaluation
       got us here */
    TValue si = kget_csi(K);
    if (!ttisnil(si)) {
        krooted_tvs_push(K, new_op);
        kset_source_info(K, new_op, si);
        krooted_tvs_pop(K);
    }
#endif

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, new_op);
}

void do_vau(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(denv);

    /*
    ** xparams[0]: op_ptree
    ** xparams[1]: penv
    ** xparams[2]: body
    ** xparams[3]: senv
    */
    TValue op_ptree = xparams[0];
    TValue penv = xparams[1];
    TValue body = xparams[2];
    TValue senv = xparams[3];

    /* bindings in an operative are in a child of the static env */
    TValue env = kmake_environment(K, senv);

    /* protect env */
    krooted_tvs_push(K, env); 

    match(K, env, op_ptree, ptree);
    if (!ttisignore(penv))
        kadd_binding(K, env, penv, denv);

    /* keep env in stack in case a cont has to be constructed */
    
    if (ttisnil(body)) {
        krooted_tvs_pop(K);
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
        krooted_tvs_pop(K);
        ktail_eval(K, kcar(body), env);
    }
}

/* 4.10.4 wrap */
void wrap(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

    bind_1tp(K, ptree, "combiner", ttiscombiner, comb);
    TValue new_app = kwrap(K, comb);
#if KTRACK_SI
    /* save as source code info the info from the expression whose evaluation
       got us here */
    TValue si = kget_csi(K);
    if (!ttisnil(si)) {
        krooted_tvs_push(K, new_app);
        kset_source_info(K, new_app, si);
        krooted_tvs_pop(K);
    }
#endif
    kapply_cc(K, new_app);
}

/* 4.10.5 unwrap */
void unwrap(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    (void) xparams;
    bind_1tp(K, ptree, "applicative", ttisapplicative, app);
    TValue underlying = kunwrap(app);
    kapply_cc(K, underlying);
}

/* 5.3.1 $vau */
/* DONE: above, together with 4.10.4 */
/* 5.3.2 $lambda */
void Slambda(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) xparams;
    bind_al1p(K, ptree, vptree, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, vptree, KIGNORE);
    krooted_tvs_push(K, vptree); 
    /* the body should be a list */
    check_list(K, true, vbody, NULL, NULL);
    vbody = copy_es_immutable_h(K, vbody, false);

    krooted_tvs_push(K, vbody); 

    TValue new_app = kmake_applicative(K, do_vau, 4, vptree, KIGNORE, vbody, 
                                       denv);
#if KTRACK_SI
    /* save as source code info the info from the expression whose evaluation
       got us here, both for the applicative and the underlying combiner */
    TValue si = kget_csi(K);
    
    if (!ttisnil(si)) {
        krooted_tvs_push(K, new_app);
        kset_source_info(K, new_app, si);
        kset_source_info(K, kunwrap(new_app), si);
        krooted_tvs_pop(K);
    }
#endif

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, new_app);
}

/* 5.5.1 apply */
void apply(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);

    bind_al2tp(K, ptree, 
               "applicative", ttisapplicative, app, 
               "any", anytype, obj, 
               maybe_env);

    TValue env = (get_opt_tpar(K, maybe_env, "environment", ttisenvironment))?
        maybe_env : kmake_empty_environment(K);

    krooted_tvs_push(K, env); 
    TValue expr = kcons(K, kunwrap(app), obj);
    krooted_tvs_pop(K); 
    /* TODO track source code info */
    ktail_eval(K, expr, env);
}

/* Continuation helpers for map */

/* For acyclic input lists: Return the mapped list */
void do_map_ret(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: (dummy . complete-ls)
    */
    UNUSED(obj);
    /* copy the list to avoid problems with continuations
       captured from within the dynamic extent to map
       and later mutation of the result */
    /* XXX: the check isn't necessary really, but there is
       no list_copy */
    TValue copy = check_copy_list(K, kcdr(xparams[0]), false, NULL, NULL);
    kapply_cc(K, copy);
}

/* For cyclic input list: close the cycle and return the mapped list */
void do_map_encycle(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: (dummy . complete-ls)
    ** xparams[1]: last non-cycle pair
    */
    /* obj: (rem-ls . last-pair) */
    TValue lp = kcdr(obj);
    TValue lap = xparams[1];

    TValue fcp = kcdr(lap);
    TValue lcp = lp;
    kset_cdr(lcp, fcp);

    /* copy the list to avoid problems with continuations
       captured from within the dynamic extent to map
       and later mutation of the result */
    /* XXX: the check isn't necessary really, but there is
       no list_copy */
    TValue copy = check_copy_list(K, kcdr(xparams[0]), false, NULL, NULL);
    kapply_cc(K, copy);
}

void do_map(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: app
    ** xparams[1]: rem-ls
    ** xparams[2]: last-pair
    ** xparams[3]: n
    ** xparams[4]: denv
    ** xparams[5]: dummyp
    */
    TValue app = xparams[0];
    TValue ls = xparams[1];
    TValue last_pair = xparams[2];
    int32_t n = ivalue(xparams[3]);
    TValue denv = xparams[4];
    bool dummyp = bvalue(xparams[5]);

    /* this case is used to kick start the mapping of both
       the acyclic and cyclic part, avoiding code duplication */
    if (!dummyp) {
        TValue np = kcons(K, obj, KNIL);
        kset_cdr(last_pair, np);
        last_pair = np;
    }

    if (n == 0) {
        /* pass the rest of the list and last pair for cycle handling */
        kapply_cc(K, kcons(K, ls, last_pair));
    } else {
        /* copy the ptree to avoid problems with mutation */
        /* XXX: no check necessary, could just use copy_list if there
           was such a procedure */
        TValue first_ptree = check_copy_list(K, kcar(ls), false, NULL, NULL);
        ls = kcdr(ls);
        n = n-1;
        krooted_tvs_push(K, first_ptree);
        /* have to unwrap the applicative to avoid extra evaluation of first */
        TValue new_expr = kcons(K, kunwrap(app), first_ptree);
        krooted_tvs_push(K, new_expr);
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_map, 6, app, 
                               ls, last_pair, i2tv(n), denv, KFALSE);
        krooted_tvs_pop(K); 
        krooted_tvs_pop(K); 
        kset_cc(K, new_cont);
        ktail_eval(K, new_expr, denv);
    }
}

void do_map_cycle(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: app
    ** xparams[1]: (dummy . res-list)
    ** xparams[2]: cpairs
    ** xparams[3]: denv
    */ 

    TValue app = xparams[0];
    TValue dummy = xparams[1];
    int32_t cpairs = ivalue(xparams[2]);
    TValue denv = xparams[3];

    /* obj: (cycle-part . last-result-pair) */
    TValue ls = kcar(obj);
    TValue last_apair = kcdr(obj);

    /* this continuation will close the cycle and return the list */
    TValue encycle_cont =
        kmake_continuation(K, kget_cc(K), do_map_encycle, 2, 
                           dummy, last_apair);

    krooted_tvs_push(K, encycle_cont);
    /* schedule the mapping of the elements of the cycle, 
       signal dummyp = true to avoid creating a pair for
       the inert value passed to the first continuation */
    TValue new_cont = 
        kmake_continuation(K, encycle_cont, do_map, 6, app, ls, 
                           last_apair, i2tv(cpairs), denv, KTRUE);
    klisp_assert(ttisenvironment(denv));

    krooted_tvs_pop(K); 
    kset_cc(K, new_cont);
    /* this will be like a nop and will continue with do_map */
    kapply_cc(K, KINERT);
}

/* 5.9.1 map */
void map(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);

    bind_al1tp(K, ptree, "applicative", ttisapplicative, app, lss);
    
    if (ttisnil(lss)) {
        klispE_throw_simple(K, "no lists");
        return;
    }

    /* get the metrics of the ptree of each call to app and
       of the result list */
    int32_t app_pairs, app_apairs, app_cpairs;
    int32_t res_pairs, res_apairs, res_cpairs;

    map_for_each_get_metrics(K, lss, &app_apairs, &app_cpairs,
                             &res_apairs, &res_cpairs);
    app_pairs = app_apairs + app_cpairs;
    res_pairs = res_apairs + res_cpairs;
    UNUSED(app_pairs);
    UNUSED(res_pairs);

    /* create the list of parameters to app */
    lss = map_for_each_transpose(K, lss, app_apairs, app_cpairs, 
                                 res_apairs, res_cpairs);

    /* ASK John: the semantics when this is mixed with continuations,
       isn't all that great..., but what are the expectations considering
       there is no prescribed order? */

    krooted_tvs_push(K, lss);
    /* This will be the list to be returned, but it will be copied
       before to play a little nicer with continuations */
    TValue dummy = kcons(K, KINERT, KNIL);
    
    krooted_tvs_push(K, dummy);

    TValue ret_cont = (res_cpairs == 0)?
        kmake_continuation(K, kget_cc(K), do_map_ret, 1, dummy)
        : kmake_continuation(K, kget_cc(K), do_map_cycle, 4, 
                             app, dummy, i2tv(res_cpairs), denv);

    krooted_tvs_push(K, ret_cont);

    /* schedule the mapping of the elements of the acyclic part.
       signal dummyp = true to avoid creating a pair for
       the inert value passed to the first continuation */
    TValue new_cont = 
        kmake_continuation(K, ret_cont, do_map, 6, app, lss, dummy,
                           i2tv(res_apairs), denv, KTRUE);

    krooted_tvs_pop(K); 
    krooted_tvs_pop(K); 
    krooted_tvs_pop(K); 

    kset_cc(K, new_cont);

    /* this will be a nop, and will continue with do_map */
    kapply_cc(K, KINERT);
}

/* 
** These are from r7rs (except bytevector). For now just follow
** Kernel version of (list) map. That means that the objects should
** all have the same size, and that the dynamic environment is passed
** to the applicatives. Continuation capturing interaction is still
** an open issue (see comment in map).
*/

/* NOTE: the type error on the result of app are only checked after
   all values are collected. This could be changed if necessary, by
   having map continuations take an additional typecheck param */
/* Helpers for array_map */

/* copy the resulting list to a new vector */
void do_array_map_ret(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: (dummy . complete-ls)
    ** xparams[1]: list->array
    ** xparams[2]: length
    */
    UNUSED(obj);

    TValue ls = kcdr(xparams[0]);
    TValue (*list_to_array)(klisp_State *K, TValue array, int32_t size) = 
        pvalue(xparams[1]);
    int32_t length = ivalue(xparams[2]);

    /* This will also avoid some problems with continuations
       captured from within the dynamic extent to map
       and later mutation of the result */
    TValue copy = list_to_array(K, ls, length);
    kapply_cc(K, copy);
}

/* 5.9.? string-map */
/* 5.9.? vector-map */
/* 5.9.? bytevector-map */
void array_map(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    /*
    ** xparams[0]: list->array fn 
    ** xparams[1]: array->list fn (with type check and size ret)
    */

    TValue list_to_array_tv = xparams[0];
    TValue (*array_to_list)(klisp_State *K, TValue array, int32_t *size) = 
        pvalue(xparams[1]);

    bind_al1tp(K, ptree, "applicative", ttisapplicative, app, lss);
    
    /* check that lss is a non empty list, and copy it */
    if (ttisnil(lss)) {
        klispE_throw_simple(K, "no arguments after applicative");
        return;
    }

    int32_t app_pairs, app_apairs, app_cpairs;
    /* the copied list should be protected from gc, and will host
       the lists resulting from the conversion */
    lss = check_copy_list(K, lss, true, &app_pairs, &app_cpairs);
    app_apairs = app_pairs - app_cpairs;
    krooted_tvs_push(K, lss);

    /* check that all elements have the correct type and same size,
       and convert them to lists */
    int32_t res_pairs;
    TValue head = kcar(lss);
    TValue tail = kcdr(lss);
    TValue ls = array_to_list(K, head, &res_pairs);
    kset_car(lss, ls); /* save the first */
    /* all array will produce acyclic lists */

    for(int32_t i = 1 /* jump over first */; i < app_pairs; ++i) {
        head = kcar(tail);
        int32_t pairs;
        ls = array_to_list(K, head, &pairs);
        /* in klisp all arrays should have the same length */
        if (pairs != res_pairs) {
            klispE_throw_simple(K, "arguments of different length");
            return;
        }
        kset_car(tail, ls);
        tail = kcdr(tail);
    }
    
    /* create the list of parameters to app */
    lss = map_for_each_transpose(K, lss, app_apairs, app_cpairs, 
                                 res_pairs, 0); /* cycle pairs is always 0 */

    /* ASK John: the semantics when this is mixed with continuations,
       isn't all that great..., but what are the expectations considering
       there is no prescribed order? */

    krooted_tvs_pop(K);
    krooted_tvs_push(K, lss);
    /* This will be the list to be returned, but it will be transformed
       to an array before returning (making it also play a little nicer 
       with continuations) */
    TValue dummy = kcons(K, KINERT, KNIL);
    
    krooted_tvs_push(K, dummy);

    TValue ret_cont = 
        kmake_continuation(K, kget_cc(K), do_array_map_ret, 3, dummy, 
                           list_to_array_tv, i2tv(res_pairs));
    krooted_tvs_push(K, ret_cont);

    /* schedule the mapping of the elements of the acyclic part.
       signal dummyp = true to avoid creating a pair for
       the inert value passed to the first continuation */
    TValue new_cont = 
        kmake_continuation(K, ret_cont, do_map, 6, app, lss, dummy,
                           i2tv(res_pairs), denv, KTRUE);

    krooted_tvs_pop(K); 
    krooted_tvs_pop(K); 
    krooted_tvs_pop(K); 

    kset_cc(K, new_cont);

    /* this will be a nop, and will continue with do_map */
    kapply_cc(K, KINERT);
}

/* 6.2.1 combiner? */
/* uses ftypedp */

/* init ground */
void kinit_combiners_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /* 4.10.1 operative? */
    add_applicative(K, ground_env, "operative?", typep, 2, symbol, 
                    i2tv(K_TOPERATIVE));
    /* 4.10.2 applicative? */
    add_applicative(K, ground_env, "applicative?", typep, 2, symbol, 
                    i2tv(K_TAPPLICATIVE));
    /* 4.10.3 $vau */
    /* 5.3.1 $vau */
    add_operative(K, ground_env, "$vau", Svau, 0);
    /* 4.10.4 wrap */
    add_applicative(K, ground_env, "wrap", wrap, 0);
    /* 4.10.5 unwrap */
    add_applicative(K, ground_env, "unwrap", unwrap, 0);
    /* 5.3.2 $lambda */
    add_operative(K, ground_env, "$lambda", Slambda, 0);
    /* 5.5.1 apply */
    add_applicative(K, ground_env, "apply", apply, 0);
    /* 5.9.1 map */
    add_applicative(K, ground_env, "map", map, 0);
    /* 5.9.? string-map, vector-map, bytevector-map */
    add_applicative(K, ground_env, "string-map", array_map, 2, 
                    p2tv(list_to_string_h), p2tv(string_to_list_h));
    add_applicative(K, ground_env, "vector-map", array_map, 2, 
                    p2tv(list_to_vector_h), p2tv(vector_to_list_h));
    add_applicative(K, ground_env, "bytevector-map", array_map, 2, 
                    p2tv(list_to_bytevector_h), p2tv(bytevector_to_list_h));
    /* 6.2.1 combiner? */
    add_applicative(K, ground_env, "combiner?", ftypep, 2, symbol, 
                    p2tv(kcombinerp));
}

/* XXX lock? */
/* init continuation names */
void kinit_combiners_cont_names(klisp_State *K)
{
    Table *t = tv2table(G(K)->cont_name_table);
    
    add_cont_name(K, t, do_vau, "$vau-bind!-eval");

    add_cont_name(K, t, do_map, "map-acyclic-part");
    add_cont_name(K, t, do_map_encycle, "map-encycle!");
    add_cont_name(K, t, do_map_ret, "map-ret");
    add_cont_name(K, t, do_map_cycle, "map-cyclic-part");

    add_cont_name(K, t, do_array_map_ret, "array-map-ret");
}
