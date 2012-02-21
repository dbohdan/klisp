/*
** kgenv_mut.c
** Environment mutation features for the ground environment
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

#include "kghelpers.h"
#include "kgenv_mut.h"

/* Continuations */
void do_match(klisp_State *K);
void do_set_eval_obj(klisp_State *K);
void do_import(klisp_State *K);

/* 4.9.1 $define! */
void SdefineB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0] = define symbol
    */
    bind_2p(K, ptree, dptree, expr);
    
    TValue def_sym = xparams[0];

    dptree = check_copy_ptree(K, dptree, KIGNORE);

    krooted_tvs_push(K, dptree);
	
    TValue new_cont = kmake_continuation(K, kget_cc(K),
                                         do_match, 3, dptree, denv, 
                                         def_sym);
    kset_cc(K, new_cont);
    krooted_tvs_pop(K);
    ktail_eval(K, expr, denv);
}

/* helper */
void do_match(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: ptree
    ** xparams[1]: dynamic environment
    ** xparams[2]: combiner symbol
    */
    TValue ptree = xparams[0];
    TValue env = xparams[1];

    match(K, env, ptree, obj);
    kapply_cc(K, KINERT);
}

/* 6.8.1 $set! */
void SsetB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);

    TValue sname = xparams[0];

    bind_3p(K, ptree, env_exp, raw_formals, eval_exp);

    TValue formals = check_copy_ptree(K, raw_formals, KIGNORE);
    krooted_tvs_push(K, formals);

    TValue new_cont = 
        kmake_continuation(K, kget_cc(K), do_set_eval_obj, 4, 
                           sname, formals, eval_exp, denv);
    kset_cc(K, new_cont);

    krooted_tvs_pop(K);
    ktail_eval(K, env_exp, denv);
}

/* Helpers for $set! */
void do_set_eval_obj(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: name as symbol
    ** xparams[1]: ptree
    ** xparams[2]: expression to be eval'ed
    ** xparams[3]: dynamic environment
    */
    TValue sname = xparams[0];
    TValue formals = xparams[1];
    TValue eval_exp = xparams[2];
    TValue denv = xparams[3];
    
    if (!ttisenvironment(obj)) {
        klispE_throw_simple(K, "bad type from first "
                            "operand evaluation (expected environment)");
        return;
    } else {
        TValue env = obj;

        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_match, 3, 
                               formals, env, sname);
        kset_cc(K, new_cont);
        ktail_eval(K, eval_exp, denv);
    }
}

/* Helpers for $provide! & $import! */

static inline void unmark_maybe_symbol_list(klisp_State *K, TValue ls)
{
    UNUSED(K);
    while(ttispair(ls) && kis_marked(ls)) {
        TValue first = kcar(ls);
        if (ttissymbol(first))
            kunmark_symbol(first);
        kunmark(ls);
        ls = kcdr(ls);
    }
}

/* 
** Check that obj is a finite list of symbols with no duplicates and
** returns a copy of the list (cf. check_copy_ptree)
*/
/* GC: Assumes obj is rooted */
TValue check_copy_symbol_list(klisp_State *K, TValue obj)
{
    TValue tail = obj;
    bool type_errorp = false;
    bool repeated_errorp = false;
    TValue slist = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &slist);
    TValue last_pair = slist;

    while(ttispair(tail) && !kis_marked(tail)) {
        /* even if there is a type error continue checking the structure */
        TValue first = kcar(tail);
        if (ttissymbol(first)) {
            repeated_errorp |= kis_symbol_marked(first);
            kmark_symbol(first);
        } else {
            type_errorp = true;
        }
        kmark(tail);

        TValue new_pair = kcons(K, first, KNIL);
        kset_cdr(last_pair, new_pair);
        last_pair = new_pair;

        tail = kcdr(tail);
    }
    unmark_maybe_symbol_list(K, obj);

    if (!ttisnil(tail)) {
        klispE_throw_simple(K, "expected finite list"); 
        return KNIL;
    } else if (type_errorp) {
        klispE_throw_simple(K, "bad operand type (expected list of "
                            "symbols)"); 
        return KNIL;
    } else if (repeated_errorp) {
        klispE_throw_simple(K, "repeated symbols");
    }
    krooted_vars_pop(K);
    return kcdr(slist);
}

void do_import(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: name as symbol
    ** xparams[1]: symbols
    ** xparams[2]: dynamic environment
    */
    TValue sname = xparams[0];
    TValue symbols = xparams[1];
    TValue denv = xparams[2];
    
    if (!ttisenvironment(obj)) {
        klispE_throw_simple(K, "bad type from first "
                            "operand evaluation (expected environment)");
        return;
    } else {
        TValue env = obj;
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_match, 3, 
                               symbols, denv, sname);
        kset_cc(K, new_cont);
        ktail_eval(K, kcons(K, K->list_app, symbols), env);
    }
}

/* 6.8.2 $provide! */
void SprovideB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /* 
    ** xparams[0]: name as symbol
    */
    TValue sname = xparams[0];

    bind_al1p(K, ptree, symbols, body);

    symbols = check_copy_symbol_list(K, symbols);
    krooted_tvs_push(K, symbols);
    body = check_copy_list(K, body, false, NULL, NULL);
    krooted_tvs_push(K, body);
    
    TValue new_env = kmake_environment(K, denv);
    /* this will copy the bindings from new_env to denv */
    krooted_tvs_push(K, new_env);
    TValue import_cont =
        kmake_continuation(K, kget_cc(K), do_import, 3, 
                           sname, symbols, denv);
    kset_cc(K, import_cont); /* this implicitly roots import_cont */
    /* this will ignore the last value and pass the env to the 
       above continuation */
    TValue ret_exp_cont = 
        kmake_continuation(K, import_cont, do_return_value, 
                           1, new_env);
    kset_cc(K, ret_exp_cont); /* this implicitly roots ret_exp_cont */

    if (ttisnil(body)) {
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        kapply_cc(K, KINERT);
    } else {
        /* this is needed because seq continuation doesn't check for 
           nil sequence */
        TValue tail = kcdr(body);
        if (ttispair(tail)) {
            TValue new_cont = kmake_continuation(K, kget_cc(K),
                                                 do_seq, 2, tail, new_env);
            kset_cc(K, new_cont);
#if KTRACK_SI
            /* put the source info of the list including the element
               that we are about to evaluate */
            kset_source_info(K, new_cont, ktry_get_si(K, body));
#endif
        } 
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        krooted_tvs_pop(K);
        ktail_eval(K, kcar(body), new_env);
    }
}

/* 6.8.3 $import! */
void SimportB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /* ASK John: The report says that symbols can have repeated symbols
       and even be cyclical (cf $provide!) however this doesn't work
       in the derivation (that uses $set! and so needs a ptree, which are
       acyclical and with no repeated symbols).
       Here I follow $provide! and don't allow repeated symbols or cyclical
       lists, NOTE: is this restriction is to be lifted the code to copy the
       list should guarantee to contruct an acyclical list or do_import be
       changed to work with cyclical lists (at the moment it uses do_match
       that expects a ptree (although it works with repeated symbols provided
       they all have the same value, it loops indefinitely with cyclical ptree) 
    */
    /* 
    ** xparams[0]: name as symbol
    */
    TValue sname = xparams[0];

    bind_al1p(K, ptree, env_expr, symbols);

    symbols = check_copy_symbol_list(K, symbols);
    
    /* REFACTOR/ASK John: another way for this kind of operative would be
       to first eval the env expression and only then check the type
       of the symbol list (other operatives that could use this model to
       avoid copying are $set!, $define! & $binds?) */

    krooted_tvs_push(K, symbols);
    TValue new_cont =
	    kmake_continuation(K, kget_cc(K), do_import, 3, 
                           sname, symbols, denv);
    kset_cc(K, new_cont);
    krooted_tvs_pop(K);
    ktail_eval(K, env_expr, denv);
}

/* init ground */
void kinit_env_mut_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 4.9.1 $define! */
    add_operative(K, ground_env, "$define!", SdefineB, 1, symbol);
    /* 6.8.1 $set! */
    add_operative(K, ground_env, "$set!", SsetB, 1, symbol);
    /* 6.8.2 $provide! */
    add_operative(K, ground_env, "$provide!", SprovideB, 1, symbol);
    /* 6.8.3 $import! */
    add_operative(K, ground_env, "$import!", SimportB, 1, symbol);
}

/* init continuation names */
void kinit_env_mut_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);

    add_cont_name(K, t, do_match, "match-ptree");
    add_cont_name(K, t, do_set_eval_obj, "set-eval-obj");
    add_cont_name(K, t, do_import, "import-bindings");
}

