/*
** kgmodules.c
** Modules features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kmodule.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kpair.h"
#include "kenvironment.h"

#include "kghelpers.h"
#include "kgmodules.h"

/* Continuations */
void do_module_registration(klisp_State *K);


/* ?.? module? */
/* uses typep */

/* Helper for make-module */
inline void unmark_symbol_list(klisp_State *K, TValue ls)
{
    UNUSED(K);
    while(ttispair(ls) && kis_symbol_marked(kcar(ls))) {
        kunmark_symbol(kcar(ls));
        ls = kcdr(ls);
    }
}

/* ?.? make-module */
void make_module(klisp_State *K)
{
    bind_1p(K, K->next_value, obj);

    int32_t pairs;
    /* list can't be cyclical */
    check_list(K, false, obj, &pairs, NULL);
    /* 
    ** - check the type (also check symbols aren't repeated)
    ** - copy the symbols in an immutable list 
    ** - put the values in a new empty env 
    */
    TValue dummy = kcons(K, KNIL, KNIL);
    krooted_tvs_push(K, dummy);
    TValue lp = dummy;
    TValue tail = obj;
    /* use a table environment for modules */
    TValue env = kmake_table_environment(K, KNIL);
    krooted_tvs_push(K, env);

    for (int32_t i = 0; i < pairs; ++i, tail = kcdr(tail)) {
        TValue p = kcar(tail);
        if (!ttispair(p) || !ttissymbol(kcar(p))) {
            unmark_symbol_list(K, kcdr(dummy));
            klispE_throw_simple_with_irritants(K, "Bad type in bindings",
                                               1, tail);
            return;
        }

        TValue sym = kcar(p);
        TValue val = kcdr(p);
        if (kis_symbol_marked(sym)) {
            unmark_symbol_list(K, kcdr(dummy));
            klispE_throw_simple_with_irritants(K, "Repeated symbol in "
                                               "bindings", 1, sym);
            return;
        }
        kmark_symbol(sym);

        TValue np = kimm_cons(K, sym, KNIL);
        kset_cdr_unsafe(K, lp, np);
        lp = np;
        kadd_binding(K, env, sym, val);
    }

    unmark_symbol_list(K, kcdr(dummy));
    TValue new_mod = kmake_module(K, env, kcdr(dummy));
    krooted_tvs_pop(K); krooted_tvs_pop(K);
    kapply_cc(K, new_mod);
}

/* ?.? get-module-export-list */
void get_module_export_list(klisp_State *K)
{
    bind_1tp(K, K->next_value, "module", ttismodule, mod);
    /* return mutable list (following the Kernel report) */
    /* XXX could use unchecked_copy_list if available */
    TValue copy = check_copy_list(K, kmodule_exp_list(mod), true, NULL, NULL);
    kapply_cc(K, copy);
}

/* ?.? get-module-environment */
void get_module_environment(klisp_State *K)
{
    bind_1tp(K, K->next_value, "module", ttismodule, mod);
    kapply_cc(K, kmake_environment(K, kmodule_env(mod)));
}

/* Helpers for working with module names */
bool valid_name_partp(TValue obj)
{
    return ttissymbol(obj) || (keintegerp(obj) && !knegativep(obj));
}

void check_module_name(klisp_State *K, TValue name)
{
    check_typed_list(K, valid_name_partp, false, name, NULL, NULL);
}

TValue modules_registry_assoc(klisp_State *K, TValue name, TValue *lastp)
{
    TValue last = KNIL;
    TValue res = KNIL;
    for (TValue ls = K->modules_registry; !ttisnil(ls); last = ls, 
             ls = kcdr(ls)) {
        if (equal2p(K, kcar(kcar(ls)), name)) {
            res = kcar(ls);
            break;
        }
    }
    if (lastp != NULL) *lastp = last;
    return res;
}

/* ?.? $registered-module? */
void Sregistered_moduleP(klisp_State *K)
{
    bind_1p(K, K->next_value, name);
    check_module_name(K, name);
    TValue entry = modules_registry_assoc(K, name, NULL);
    kapply_cc(K, ttisnil(entry)? KFALSE : KTRUE);
}

/* ?.? $get-registered-module */
void Sget_registered_module(klisp_State *K)
{
    bind_1p(K, K->next_value, name);
    check_module_name(K, name);
    TValue entry = modules_registry_assoc(K, name, NULL);
    if (ttisnil(entry)) {
        klispE_throw_simple_with_irritants(K, "Unregistered module name",
                                           1, name);
        return;
    }
    kapply_cc(K, kcdr(entry));
}

void do_module_registration(klisp_State *K)
{
    /* 
    ** xparams[0]: name 
    */
    TValue obj = K->next_value;
    if (!ttismodule(obj)) {
        klispE_throw_simple_with_irritants(K, "not a module", 1, obj);
        return;
    }
    TValue name = K->next_xparams[0];
    TValue entry = modules_registry_assoc(K, name, NULL);
    if (!ttisnil(entry)) {
        klispE_throw_simple_with_irritants(K, "module name already registered",
                                           1, name);
        return;
    }
    TValue np = kcons(K, name, obj);
    krooted_tvs_push(K, np);
    np = kcons(K, np, K->modules_registry);
    K->modules_registry = np;
    krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* ?.? $register-module! */
void Sregister_moduleB(klisp_State *K)
{
    bind_2p(K, K->next_value, name, module);
    check_module_name(K, name);
    /* copy the name to avoid mutation */
    /* XXX could use unchecked_copy_list if available */
    name = check_copy_list(K, name, false, NULL, NULL);
    krooted_tvs_push(K, name);
    TValue cont = kmake_continuation(K, kget_cc(K), do_module_registration,
                                     1, name);
    krooted_tvs_pop(K);
    kset_cc(K, cont);
    ktail_eval(K, module, K->next_env);
}

/* ?.? $unregister-module! */
void Sunregister_moduleB(klisp_State *K)
{
    bind_1p(K, K->next_value, name);
    check_module_name(K, name);
    TValue last;
    TValue entry = modules_registry_assoc(K, name, &last);
    if (ttisnil(entry)) {
        klispE_throw_simple_with_irritants(K, "module name not registered",
                                           1, name);
        return;
    }
    if (ttisnil(last)) { /* it's in the first pair */
        K->modules_registry = kcdr(K->modules_registry);
    } else {
        kset_cdr(last, kcdr(kcdr(last)));
    }
    kapply_cc(K, KINERT);
}

/* init ground */
void kinit_modules_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    add_applicative(K, ground_env, "module?", typep, 2, symbol, 
                    i2tv(K_TMODULE));
    add_applicative(K, ground_env, "make-module", make_module, 0); 
    add_applicative(K, ground_env, "get-module-export-list", 
                    get_module_export_list, 0); 
    add_applicative(K, ground_env, "get-module-environment", 
                    get_module_environment, 0); 
    add_operative(K, ground_env, "$registered-module?", Sregistered_moduleP, 
                  0);
    add_operative(K, ground_env, "$get-registered-module", 
                  Sget_registered_module, 0);
    add_operative(K, ground_env, "$register-module!", Sregister_moduleB, 
                  0);
    add_operative(K, ground_env, "$unregister-module!", Sunregister_moduleB, 
                  0);
}

/* init continuation names */
void kinit_modules_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);

    add_cont_name(K, t, do_module_registration, "register-module"); 
}
