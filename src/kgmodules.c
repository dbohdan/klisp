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
#include "kkeyword.h"

#include "kghelpers.h"
#include "kgmodules.h"

/* Continuations */
void do_register_module(klisp_State *K);
void do_provide_module(klisp_State *K);


/* ?.? module? */
/* uses typep */

/* Helper for make-module */
inline void unmark_symbol_list(klisp_State *K, TValue ls)
{
    UNUSED(K);
    for(; ttispair(ls) && kis_symbol_marked(kcar(ls)); ls = kcdr(ls))
        kunmark_symbol(kcar(ls));
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

void do_register_module(klisp_State *K)
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
    TValue cont = kmake_continuation(K, kget_cc(K), do_register_module,
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

/* Helpers for provide-module */
void unmark_export_list(klisp_State *K, TValue exports, TValue last)
{
    /* exports shouldn't have the leading keyword */
    UNUSED(K);
    for (; !tv_equal(exports, last); exports = kcdr(exports)) {
        TValue first = kcar(exports);
        if (ttissymbol(first))
            kunmark_symbol(first);
        else
            kunmark_symbol(kcar(kcdr(kcdr(first))));
    }
}

void check_export_list(klisp_State *K, TValue exports)
{
    int32_t pairs;
    check_list(K, false, exports, &pairs, NULL);
    if (ttisnil(exports) || !ttiskeyword(kcar(exports)) ||
        kkeyword_cstr_cmp(kcar(exports), "export") != 0) {

        klispE_throw_simple_with_irritants(K, "missing #:export keyword",
                                           1, exports);
        return;
    }
    /* empty export list are allowed (but still need #:export) */
    --pairs;
    exports = kcdr(exports);
    /* check that all entries are either a unique symbol or
       a rename form: (#:rename int-s ext-s) with unique ext-s */
    for (TValue tail = exports; pairs > 0; --pairs, tail = kcdr(tail)) {
        TValue clause = kcar(tail);
        TValue symbol;
        if (ttissymbol(clause)) {
            symbol = clause;
        } else {
            int32_t pairs;
            /* this use of marks doesn't interfere with symbols */
            check_list(K, false, clause, &pairs, NULL);
            if (pairs != 3 || 
                kkeyword_cstr_cmp(kcar(clause), "rename") != 0) {

                unmark_export_list(K, exports, tail);
                klispE_throw_simple_with_irritants(K, "Bad export clause "
                                                   "syntax", 1, clause);
                return;
            } else if (!ttissymbol(kcar(kcdr(clause))) || 
                       !ttissymbol(kcar(kcdr(kcdr(clause))))) {
                unmark_export_list(K, exports, tail);
                klispE_throw_simple_with_irritants(K, "Non symbol in #:rename "
                                                   "export clause", 1, clause);
                return;
            } else {
                symbol = kcar(kcdr(kcdr(clause)));
            }
        } 

        if (kis_symbol_marked(symbol)) {
            unmark_export_list(K, exports, tail);
            klispE_throw_simple_with_irritants(K, "repeated symbol in export "
                                               "list", 1, symbol);
            return;
        }
        kmark_symbol(symbol);
    }
    unmark_export_list(K, exports, KNIL);
}

void do_provide_module(klisp_State *K)
{
    /* 
    ** xparams[0]: name 
    ** xparams[1]: inames
    ** xparams[2]: enames
    ** xparams[3]: env
    */
    TValue name = K->next_xparams[0];

    if (!ttisnil(modules_registry_assoc(K, name, NULL))) {
        klispE_throw_simple_with_irritants(K, "module name already registered",
                                           1, name);
        return;
    }

    TValue inames = K->next_xparams[1];
    TValue enames = K->next_xparams[2];
    TValue env = K->next_xparams[3];

    TValue new_env = kmake_table_environment(K, KNIL);
    krooted_tvs_push(K, new_env);

    for (; !ttisnil(inames); inames = kcdr(inames), enames = kcdr(enames)) {
        TValue iname = kcar(inames);
        if (!kbinds(K, env, iname)) {
            klispE_throw_simple_with_irritants(K, "unbound exported symbol in "
                                               "module", 1, iname);
            return;
        }
        kadd_binding(K, new_env, kcar(enames), kget_binding(K, env, iname));
    }

    enames = K->next_xparams[2];
    TValue module = kmake_module(K, new_env, enames);
    krooted_tvs_pop(K); /* new_env */
    krooted_tvs_push(K, module);

    TValue np = kcons(K, name, module);
    krooted_tvs_pop(K); /* module */
    krooted_tvs_push(K, np);
    np = kcons(K, np, K->modules_registry);
    K->modules_registry = np;
    krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* ?.? $provide-module! */
void Sprovide_moduleB(klisp_State *K)
{
    bind_al2p(K, K->next_value, name, exports, body);
    check_module_name(K, name);
    name = check_copy_list(K, name, false, NULL, NULL);
    krooted_tvs_push(K, name);
    check_export_list(K, exports);
    TValue inames = kimm_cons(K, KNIL, KNIL);
    TValue ilast = inames;
    krooted_vars_push(K, &inames);
    TValue enames = kimm_cons(K, KNIL, KNIL);
    TValue elast = enames;
    krooted_vars_push(K, &enames);

    for (exports = kcdr(exports); !ttisnil(exports); exports = kcdr(exports)) {
        TValue clause = kcar(exports);
        TValue isym, esym;
        if (ttissymbol(clause)) {
            isym = esym = clause;
        } else {
            isym = kcar(kcdr(clause));
            esym = kcar(kcdr(kcdr(clause)));
        }
        TValue np = kimm_cons(K, isym, KNIL);
        kset_cdr_unsafe(K, ilast, np);
        ilast = np;
        np = kimm_cons(K, esym, KNIL);
        kset_cdr_unsafe(K, elast, np);
        elast = np;
    }
    inames = kcdr(inames);
    enames = kcdr(enames);
    
    check_list(K, false, body, NULL, NULL);

    body = copy_es_immutable_h(K, body, false);
    krooted_tvs_push(K, body);

    if (!ttisnil(modules_registry_assoc(K, name, NULL))) {
        klispE_throw_simple_with_irritants(K, "module name already registered",
                                           1, name);
        return;
    }
    /* TODO add some continuation protection/additional checks */
    /* TODO add cyclical definition handling */
    // do cont

    /* use a child of the dynamic environment to do evaluations */
    TValue env = kmake_table_environment(K, K->next_env);
    krooted_tvs_push(K, env);

    kset_cc(K, kmake_continuation(K, kget_cc(K), do_provide_module,
                                  4, name, inames, enames, env));

    if (!ttisnil(body) && !ttisnil(kcdr(body))) {
        TValue cont = kmake_continuation(K, kget_cc(K), do_seq, 2, 
                                         kcdr(body), env);
        kset_cc(K, cont);
#if KTRACK_SI
    /* put the source info of the list including the element
       that we are about to evaluate */
    kset_source_info(K, cont, ktry_get_si(K, body));
#endif
    }
    
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);
    krooted_vars_pop(K); krooted_vars_pop(K);
    
    if (ttisnil(body)) {
        kapply_cc(K, KINERT);
    } else {
        ktail_eval(K, kcar(body), env);
    }
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

    add_operative(K, ground_env, "$provide-module!", Sprovide_moduleB, 0);

}

/* init continuation names */
void kinit_modules_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);

    add_cont_name(K, t, do_register_module, "register-module"); 
    add_cont_name(K, t, do_provide_module, "provide-module"); 
}
