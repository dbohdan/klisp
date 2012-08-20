/*
** kglibraries.c
** Libraries features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "kstate.h"
#include "kobject.h"
#include "klibrary.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kpair.h"
#include "kenvironment.h"
#include "kkeyword.h"

#include "kghelpers.h"
#include "kglibraries.h"

/* Continuations */
static void do_register_library(klisp_State *K);
static void do_provide_library(klisp_State *K);


/* ?.? library? */
/* uses typep */

/* Helper for make-library */
static inline void unmark_symbol_list(klisp_State *K, TValue ls)
{
    UNUSED(K);
    for(; ttispair(ls) && kis_symbol_marked(kcar(ls)); ls = kcdr(ls))
        kunmark_symbol(kcar(ls));
}

/* ?.? make-library */
static void make_library(klisp_State *K)
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
    /* use a table environment for libraries */
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
    TValue new_lib = kmake_library(K, env, kcdr(dummy));
    krooted_tvs_pop(K); krooted_tvs_pop(K);
    kapply_cc(K, new_lib);
}

/* ?.? get-library-export-list */
static void get_library_export_list(klisp_State *K)
{
    bind_1tp(K, K->next_value, "library", ttislibrary, lib);
    /* return mutable list (following the Kernel report) */
    /* XXX could use unchecked_copy_list if available */
    TValue copy = check_copy_list(K, klibrary_exp_list(lib), true, NULL, NULL);
    kapply_cc(K, copy);
}

/* ?.? get-library-environment */
static void get_library_environment(klisp_State *K)
{
    bind_1tp(K, K->next_value, "library", ttislibrary, lib);
    kapply_cc(K, kmake_environment(K, klibrary_env(lib)));
}

/* Helpers for working with library names */
static bool valid_name_partp(TValue obj)
{
    return ttissymbol(obj) || (keintegerp(obj) && !knegativep(obj));
}

static void check_library_name(klisp_State *K, TValue name)
{
    if (ttisnil(name)) {
        klispE_throw_simple(K, "Empty library name");
        return;
    }
    check_typed_list(K, valid_name_partp, false, name, NULL, NULL);
}

static TValue libraries_registry_assoc(klisp_State *K, TValue name, TValue *lastp)
{
    TValue last = KNIL;
    TValue res = KNIL;
    for (TValue ls = G(K)->libraries_registry; !ttisnil(ls); last = ls, 
             ls = kcdr(ls)) {
        if (equal2p(K, kcar(kcar(ls)), name)) {
            res = kcar(ls);
            break;
        }
    }
    if (lastp != NULL) *lastp = last;
    return res;
}

/* ?.? $registered-library? */
static void Sregistered_libraryP(klisp_State *K)
{
    bind_1p(K, K->next_value, name);
    check_library_name(K, name);
    TValue entry = libraries_registry_assoc(K, name, NULL);
    kapply_cc(K, ttisnil(entry)? KFALSE : KTRUE);
}

/* ?.? $get-registered-library */
static void Sget_registered_library(klisp_State *K)
{
    bind_1p(K, K->next_value, name);
    check_library_name(K, name);
    TValue entry = libraries_registry_assoc(K, name, NULL);
    if (ttisnil(entry)) {
        klispE_throw_simple_with_irritants(K, "Unregistered library name",
                                           1, name);
        return;
    }
    kapply_cc(K, kcdr(entry));
}

static void do_register_library(klisp_State *K)
{
    /* 
    ** xparams[0]: name 
    */
    TValue obj = K->next_value;
    if (!ttislibrary(obj)) {
        klispE_throw_simple_with_irritants(K, "not a library", 1, obj);
        return;
    }
    TValue name = K->next_xparams[0];
    TValue entry = libraries_registry_assoc(K, name, NULL);
    if (!ttisnil(entry)) {
        klispE_throw_simple_with_irritants(K, "library name already registered",
                                           1, name);
        return;
    }
    TValue np = kcons(K, name, obj);
    krooted_tvs_push(K, np);
    np = kcons(K, np, G(K)->libraries_registry);
    G(K)->libraries_registry = np;
    krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* ?.? $register-library! */
static void Sregister_libraryB(klisp_State *K)
{
    bind_2p(K, K->next_value, name, library);
    check_library_name(K, name);
    /* copy the name to avoid mutation */
    /* XXX could use unchecked_copy_list if available */
    name = check_copy_list(K, name, false, NULL, NULL);
    krooted_tvs_push(K, name);
    TValue cont = kmake_continuation(K, kget_cc(K), do_register_library,
                                     1, name);
    krooted_tvs_pop(K);
    kset_cc(K, cont);
    ktail_eval(K, library, K->next_env);
}

/* ?.? $unregister-library! */
static void Sunregister_libraryB(klisp_State *K)
{
    bind_1p(K, K->next_value, name);
    check_library_name(K, name);
    TValue last;
    TValue entry = libraries_registry_assoc(K, name, &last);
    if (ttisnil(entry)) {
        klispE_throw_simple_with_irritants(K, "library name not registered",
                                           1, name);
        return;
    }
    if (ttisnil(last)) { /* it's in the first pair */
        G(K)->libraries_registry = kcdr(G(K)->libraries_registry);
    } else {
        kset_cdr(last, kcdr(kcdr(last)));
    }
    kapply_cc(K, KINERT);
}

/* Helpers for provide-library */
static void unmark_export_list(klisp_State *K, TValue exports, TValue last)
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

static void check_export_list(klisp_State *K, TValue exports)
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

static void do_provide_library(klisp_State *K)
{
    /* 
    ** xparams[0]: name 
    ** xparams[1]: inames
    ** xparams[2]: enames
    ** xparams[3]: env
    */
    TValue name = K->next_xparams[0];

    if (!ttisnil(libraries_registry_assoc(K, name, NULL))) {
        klispE_throw_simple_with_irritants(K, "library name already registered",
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
                                               "library", 1, iname);
            return;
        }
        kadd_binding(K, new_env, kcar(enames), kget_binding(K, env, iname));
    }

    enames = K->next_xparams[2];
    TValue library = kmake_library(K, new_env, enames);
    krooted_tvs_pop(K); /* new_env */
    krooted_tvs_push(K, library);

    TValue np = kcons(K, name, library);
    krooted_tvs_pop(K); /* library */
    krooted_tvs_push(K, np);
    np = kcons(K, np, G(K)->libraries_registry);
    G(K)->libraries_registry = np;
    krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* ?.? $provide-library! */
static void Sprovide_libraryB(klisp_State *K)
{
    bind_al2p(K, K->next_value, name, exports, body);
    check_library_name(K, name);
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

    if (!ttisnil(libraries_registry_assoc(K, name, NULL))) {
        klispE_throw_simple_with_irritants(K, "library name already registered",
                                           1, name);
        return;
    }
    /* TODO add some continuation protection/additional checks */
    /* TODO add cyclical definition handling */
    // do cont

    /* use a child of the dynamic environment to do evaluations */
    TValue env = kmake_table_environment(K, K->next_env);
    krooted_tvs_push(K, env);

    kset_cc(K, kmake_continuation(K, kget_cc(K), do_provide_library,
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

/* Helpers from $import-library! */

/* This takes a keyword import clause */
static void check_distinct_symbols(klisp_State *K, TValue clause)
{
    /* probably no need to use a table environment for this */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    bool pairp = kkeyword_cstr_cmp(kcar(clause), "rename") == 0;
    for (TValue ls = kcdr(kcdr(clause)); !ttisnil(ls); ls = kcdr(ls)) {
        TValue s = kcar(ls);
        TValue s2 = s;
        if (pairp) {
            if (!ttispair(s) || !ttispair(kcdr(s)) || 
                !ttisnil(kcdr(kcdr(s)))) {

                klispE_throw_simple_with_irritants(K, "bad syntax in #:rename "
                                                   "import clause", 1, clause);
                return;
            }
            s2 = kcar(s);
            /* s is the one that is checked for repeats */
            s = kcar(kcdr(s));
        }
        if (!ttissymbol(s) || !ttissymbol(s2)) {
            klispE_throw_simple_with_irritants(
                K, "Not a symbol in import clause", 1, ttissymbol(s)? s2 : s);
            return;
        } else if (kbinds(K, env, s)) {
            klispE_throw_simple_with_irritants(K, "Repeated symbol in import "
                                               "clause", 1, s);
            return;
        }
        kadd_binding(K, env, s, KINERT);
    }
    krooted_tvs_pop(K);
}

static void check_import_list(klisp_State *K, TValue imports)
{
    /* will use a stack for accumulating clauses */
    TValue stack = KNIL;
    krooted_vars_push(K, &stack);
    check_list(K, false, imports, NULL, NULL);

    while(!ttisnil(stack) || !ttisnil(imports)) {
        TValue clause;
        if (ttisnil(stack)) {
            clause = kcar(imports);
            while (ttispair(clause) && ttiskeyword(kcar(clause))) {
                stack = kcons(K, clause, stack);
                clause = kcar(kcdr(clause));
            }
            check_library_name(K, clause);
        } else {
            /* this is always a keyword clause */
            clause = kcar(stack);
            stack = kcdr(stack);
            int32_t pairs;
            check_list(K, false, clause, &pairs, NULL);
            if (pairs < 3) {
                klispE_throw_simple_with_irritants(K, "bad syntax in import "
                                                   "clause", 1, clause);
                return;
            }
            TValue keyw = kcar(clause);

            if (kkeyword_cstr_cmp(keyw, "only") == 0 ||
                kkeyword_cstr_cmp(keyw, "except") == 0 ||
                kkeyword_cstr_cmp(keyw, "rename") == 0) {
                
                check_distinct_symbols(K, clause);
            } else if (kkeyword_cstr_cmp(keyw, "prefix") == 0) {
                if (pairs != 3) {
                    klispE_throw_simple_with_irritants(K, "import clause is too "
                                                       "short", 1, clause);
                    return;
                } else if (!ttissymbol(kcar(kcdr(kcdr(clause))))) {
                    klispE_throw_simple_with_irritants(
                        K, "Non symbol in #:prefix import clause", 1, clause);
                    return;
                }
            } else {
                klispE_throw_simple_with_irritants(K, "unknown keyword in "
                                                   "import clause", 1, clause);
                return;
            }
        }
        if (ttisnil(stack))
            imports = kcdr(imports);
    }
    krooted_vars_pop(K);
}

static void check_symbols_in_bindings(klisp_State *K, TValue ls, TValue env)
{
    for (; !ttisnil(ls); ls = kcdr(ls)) {
        TValue s = kcar(ls);
        if (ttispair(s)) s = kcar(s);
        
        if (!kbinds(K, env, s)) {
            klispE_throw_simple_with_irritants(
                K, "Unknown symbol in import clause", 1, s);
            return;
        }
    }
}

static TValue extract_import_bindings(klisp_State *K, TValue imports)
{
    TValue ret_ls = kcons(K, KNIL, KNIL);
    TValue lp = ret_ls;
    krooted_tvs_push(K, ret_ls);
    TValue np = KNIL;
    krooted_vars_push(K, &np);
    /* will use a stack for accumulating clauses */
    TValue stack = KNIL;
    krooted_vars_push(K, &stack);
    TValue menv = KINERT;
    TValue mls = KINERT;
    krooted_vars_push(K, &menv);
    krooted_vars_push(K, &mls);

    while(!ttisnil(stack) || !ttisnil(imports)) {
        TValue clause;
        if (ttisnil(stack)) {
            /* clause can't be nil */
            clause = kcar(imports);
            while (ttiskeyword(kcar(clause))) {
                stack = kcons(K, clause, stack);
                clause = kcar(kcdr(clause));
            }
            TValue entry = libraries_registry_assoc(K, clause, NULL);
            if (ttisnil(entry)) {
                klispE_throw_simple_with_irritants(K, "library name not "
                                                   "registered", 1, clause);
                return KINERT;
            }
            menv = klibrary_env(kcdr(entry));
            mls = klibrary_exp_list(kcdr(entry));

            klisp_assert(ttispair(clause) && !ttiskeyword(kcar(clause)));
        } else {
            clause = kcar(stack);
            stack = kcdr(stack);
        }

        if (ttiskeyword(kcar(clause))) {
            TValue keyw = kcar(clause);
            
            TValue rest = kcdr(kcdr(clause));
            if (kkeyword_cstr_cmp(keyw, "only") == 0) {
                check_symbols_in_bindings(K, rest, menv);
                mls = rest;
            } else if (kkeyword_cstr_cmp(keyw, "except") == 0) {
                check_symbols_in_bindings(K, rest, menv);
                TValue env = kmake_empty_environment(K);
                krooted_tvs_push(K, env);
                for (TValue ls = rest; !ttisnil(ls); ls = kcdr(ls))
                    kadd_binding(K, env, kcar(ls), KINERT);
                /* filter */
                TValue nmls = kcons(K, KNIL, KNIL);
                TValue nmls_lp = nmls;
                krooted_tvs_push(K, nmls);
                for (TValue ls = mls; !ttisnil(ls); ls = kcdr(ls)) {
                    TValue s = kcar(ls);
                    if (!kbinds(K, env, s)) {
                        np = kcons(K, s, KNIL);
                        kset_cdr(nmls_lp, np);
                        nmls_lp = np;
                    }
                }
                mls = kcdr(nmls);
                krooted_tvs_pop(K); krooted_tvs_pop(K);
            } else if (kkeyword_cstr_cmp(keyw, "prefix") == 0) {
                TValue pre = kcar(rest);
                TValue obj = KNIL;
                krooted_vars_push(K, &obj);
                TValue nmls = kcons(K, KNIL, KNIL);
                TValue nmls_lp = nmls;
                krooted_tvs_push(K, nmls);
                TValue nmenv = kmake_empty_environment(K);
                krooted_tvs_push(K, nmenv);
                for (TValue ls = mls; !ttisnil(ls); ls = kcdr(ls)) {
                    TValue s = kcar(ls);
                    obj = kstring_new_s(K, ksymbol_size(pre) +
                                        ksymbol_size(s));
                    memcpy(kstring_buf(obj), ksymbol_buf(pre),
                           ksymbol_size(pre));
                    memcpy(kstring_buf(obj) + ksymbol_size(pre), 
                           ksymbol_buf(s), ksymbol_size(s));
                    /* TODO attach si */
                    obj = ksymbol_new_str(K, obj, KNIL);
                    np = kcons(K, obj, KNIL);
                    kset_cdr(nmls_lp, np);
                    nmls_lp = np;

                    kadd_binding(K, nmenv, obj, kget_binding(K, menv, s));
                }
                mls = kcdr(nmls);
                menv = nmenv;
                krooted_vars_pop(K);
                krooted_tvs_pop(K); krooted_tvs_pop(K);
            } else if (kkeyword_cstr_cmp(keyw, "rename") == 0) {
                check_distinct_symbols(K, clause);
                /* env is for renamed symbols info */
                TValue env = kmake_empty_environment(K);
                krooted_tvs_push(K, env);

                /* remember all renamed symbols info first */
                for (TValue ls = rest; !ttisnil(ls); ls = kcdr(ls)) {
                    TValue p = kcar(ls);
                    kadd_binding(K, env, kcar(p), kcar(kcdr(p)));
                }

                /* now we can construct the list and env */
                TValue nmls = kcons(K, KNIL, KNIL);
                TValue nmls_lp = nmls;
                krooted_tvs_push(K, nmls);
                TValue nmenv = kmake_empty_environment(K);
                krooted_tvs_push(K, nmenv);


                /* add all renamed symbols first */
                for (TValue ls = mls; !ttisnil(ls); ls = kcdr(ls)) {
                    TValue si = kcar(ls);
                    TValue se;
                    if (kbinds(K, env, si)) /* renamed binding */
                        se = kget_binding(K, env, si);
                    else se = si;

                    /* check that symbol wasn't already defined
                       (can happen if a binding is renamed to another binding
                       of the same library and that other binding isn't itself 
                       renamed) */
                    if (kbinds(K, nmenv, se)) {
                        klispE_throw_simple_with_irritants(
                            K, "imported a symbol twice in #:rename clause", 
                            1, se);
                        return KINERT;
                    }

                    np = kcons(K, se, KNIL);
                    kset_cdr(nmls_lp, np);
                    nmls_lp = np;

                    kadd_binding(K, nmenv, se, kget_binding(K, menv, si));
                }

                mls = kcdr(nmls);
                menv = nmenv;
                krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);
            }
        }

        if (ttisnil(stack)) {
            /* move to next import clause */
            for (TValue ls = mls; !ttisnil(ls); ls = kcdr(ls)) {
                TValue s = kcar(ls);
                np = kcons(K, s, kget_binding(K, menv, s));
                np = kcons(K, np, KNIL);
                kset_cdr(lp, np);
                lp = np;
            }
            imports = kcdr(imports);
        }
    }
    krooted_vars_pop(K); krooted_vars_pop(K); 
    krooted_vars_pop(K); krooted_vars_pop(K);
    krooted_tvs_pop(K);
    return kcdr(ret_ls);
}

/* ?.? $import-library! */
static void Simport_libraryB(klisp_State *K)
{
    TValue imports = K->next_value;
    TValue denv = K->next_env;

    check_import_list(K, imports);
    /* list of (name . value) pairs */
    TValue bindings = extract_import_bindings(K, imports);
    krooted_tvs_push(K, bindings);

    TValue env = kmake_table_environment(K, KNIL);
    krooted_tvs_push(K, env);
    TValue tail;
    for (tail = bindings; !ttisnil(tail); tail = kcdr(tail)) {
        TValue s = kcar(kcar(tail));
        TValue v = kcdr(kcar(tail));
        if (kbinds(K, env, s)) {
            TValue v2 = kget_binding(K, env, s);
            if (!eq2p(K, v, v2)) {
                klispE_throw_simple_with_irritants(
                    K, "imported a symbol twice with un-eq? values", 
                    3, s, v, v2);
                return;
            }
        } else {
            kadd_binding(K, env, s, v);
        }
    }

    for (tail = bindings; !ttisnil(tail); tail = kcdr(tail)) {
        TValue s = kcar(kcar(tail));
        TValue v = kcdr(kcar(tail));
        kadd_binding(K, denv, s, v);
    }
    krooted_tvs_pop(K); krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* init ground */
void kinit_libraries_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    add_applicative(K, ground_env, "library?", typep, 2, symbol, 
                    i2tv(K_TLIBRARY));
    add_applicative(K, ground_env, "make-library", make_library, 0); 
    add_applicative(K, ground_env, "get-library-export-list", 
                    get_library_export_list, 0); 
    add_applicative(K, ground_env, "get-library-environment", 
                    get_library_environment, 0); 

    add_operative(K, ground_env, "$registered-library?", Sregistered_libraryP, 
                  0);
    add_operative(K, ground_env, "$get-registered-library", 
                  Sget_registered_library, 0);
    add_operative(K, ground_env, "$register-library!", Sregister_libraryB, 
                  0);
    add_operative(K, ground_env, "$unregister-library!", Sunregister_libraryB, 
                  0);

    add_operative(K, ground_env, "$provide-library!", Sprovide_libraryB, 0);
    add_operative(K, ground_env, "$import-library!", Simport_libraryB, 0);
}

/* XXX lock? */
/* init continuation names */
void kinit_libraries_cont_names(klisp_State *K)
{
    Table *t = tv2table(G(K)->cont_name_table);

    add_cont_name(K, t, do_register_library, "register-library"); 
    add_cont_name(K, t, do_provide_library, "provide-library"); 
}
