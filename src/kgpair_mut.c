/*
** kgpair_mut.c
** Pair mutation features for the ground environment
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
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgpair_mut.h"

/* 4.7.1 set-car!, set-cdr! */
void set_carB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    (void) xparams;
    bind_2tp(K, ptree, "pair", ttispair, pair, 
             "any", anytype, new_car);

    if(!kis_mutable(pair)) {
	    klispE_throw_simple(K, "immutable pair");
	    return;
    }
    kset_car(pair, new_car);
    kapply_cc(K, KINERT);
}

void set_cdrB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    (void) xparams;
    bind_2tp(K, ptree, "pair", ttispair, pair, 
             "any", anytype, new_cdr);
    
    if(!kis_mutable(pair)) {
	    klispE_throw_simple(K, "immutable pair");
	    return;
    }
    kset_cdr(pair, new_cdr);
    kapply_cc(K, KINERT);
}

/* Helper for copy-es-immutable & copy-es */
void copy_es(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(denv);

    /*
    ** xparams[0]: copy-es-immutable symbol
    ** xparams[1]: boolean (#t: use mutable pairs, #f: use immutable pairs)
    */
    bool mut_flag = bvalue(xparams[1]);
    bind_1p(K, ptree, obj);

    TValue copy = copy_es_immutable_h(K, obj, mut_flag);
    kapply_cc(K, copy);
}

/* 4.7.2 copy-es-immutable */
/* uses copy_es */

/* 5.8.1 encycle! */
void encycleB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
/* ASK John: can the object be a cyclic list of length less than k1+k2? 
   the wording of the report seems to indicate that can't be the case, 
   and here it makes sense to forbid it because otherwise the list-metrics 
   of the result would differ with the expected ones (cf list-tail). 
   So here an error is signaled if the improper list cyclic with less pairs
   than needed */
    UNUSED(denv);
    UNUSED(xparams);

    bind_3tp(K, ptree, "any", anytype, obj,
             "exact integer", keintegerp, tk1,
             "exact integer", keintegerp, tk2);

    if (knegativep(tk1) || knegativep(tk2)) {
        klispE_throw_simple(K, "negative index");
        return;
    }

    if (!ttisfixint(tk1) || !ttisfixint(tk2)) {
        /* no list can have that many pairs */
        klispE_throw_simple(K, "non pair found while traversing "
                            "object");
        return;
    }

    int32_t k1 = ivalue(tk1);
    int32_t k2 = ivalue(tk2);

    TValue tail = obj;

    while(k1 != 0) {
        if (!ttispair(tail)) {
            unmark_list(K, obj);
            klispE_throw_simple(K, "non pair found while traversing "
                                "object");
            return;
        } else if (kis_marked(tail)) {
            unmark_list(K, obj);
            klispE_throw_simple(K, "too few pairs in cyclic list");
            return;
        }
        kmark(tail);
        tail = kcdr(tail);
        --k1;
    }

    TValue fcp = tail;

    /* if k2 == 0 do nothing (but this still checks that the obj
       has at least k1 pairs */
    if (k2 != 0) {
        --k2; /* to have cycle length k2 we should discard k2-1 pairs */
        /* REFACTOR: should probably refactor this to avoid the 
           duplicated checks */
        while(k2 != 0) {
            if (!ttispair(tail)) {
                unmark_list(K, obj);
                klispE_throw_simple(K, "non pair found while traversing "
                                    "object");
                return;
            } else if (kis_marked(tail)) {
                unmark_list(K, obj);
                klispE_throw_simple(K, "too few pairs in cyclic list");
                return;
            }
            kmark(tail);
            tail = kcdr(tail);
            --k2;
        }
        if (!ttispair(tail)) {
            unmark_list(K, obj);
            klispE_throw_simple(K, "non pair found while traversing "
                                "object");
            return;
        } else if (kis_marked(tail)) {
            unmark_list(K, obj);
            klispE_throw_simple(K, "too few pairs in cyclic list");
            return;
        } else if (!kis_mutable(tail)) {
            unmark_list(K, obj);
            klispE_throw_simple(K, "immutable pair");
            return;
        } else {
            kset_cdr(tail, fcp);
        }
    }
    unmark_list(K, obj);
    kapply_cc(K, KINERT);
}

/* 6.?? list-set! */
void list_setB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
/* ASK John: can the object be an improper list? 
   We foolow list-tail here and allow it */
    UNUSED(denv);
    UNUSED(xparams);

    bind_3tp(K, ptree, "any", anytype, obj,
             "exact integer", keintegerp, tk, 
             "any", anytype, val);

    if (knegativep(tk)) {
        klispE_throw_simple(K, "negative index");
        return;
    }

    int32_t k = (ttisfixint(tk))? ivalue(tk)
        : ksmallest_index(K, obj, tk);

    while(k) {
        if (!ttispair(obj)) {
            klispE_throw_simple(K, "non pair found while traversing "
                                "object");
            return;
        }
        obj = kcdr(obj);
        --k;
    }

    if (!ttispair(obj)) {
        klispE_throw_simple(K, "non pair found while traversing "
                            "object");
    } else if (kis_immutable(obj)) {
        /* this could be checked before, but the error here seems better */
        klispE_throw_simple(K, "immutable pair");
    } else {
        kset_car(obj, val);
        kapply_cc(K, KINERT);
    }
}

/* Helpers for append! */
inline void appendB_clear_last_pairs(klisp_State *K, TValue ls)
{
    UNUSED(K);
    while(ttispair(ls) && kis_marked(ls)) {
        TValue first = ls;
        ls = kget_mark(ls);
        kunmark(first);
    }
}

/* Check that all lists (except last) are acyclic lists with non repeated mutable 
   last pair (if not nil), return a list of objects so that the cdr of the odd
   objects (1 based) should be set to the next object in the list (this will
   encycle! the result if necessary) */

/* GC: Assumes lss is rooted */
TValue appendB_get_lss_endpoints(klisp_State *K, TValue lss, int32_t apairs, 
                                 int32_t cpairs)
{
    TValue elist = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &elist);
    TValue last_pair = elist;
    TValue tail = lss;
    /* this is a list of last pairs using the marks to link the pairs) */
    TValue last_pairs = KNIL;
    TValue last_apair = KNIL;

    while(apairs != 0 || cpairs != 0) {
        int32_t pairs;
	
        if (apairs == 0) {
            /* this is the first run of the loop (if there is no acyclic part) 
               or the second run of the loop (the cyclic part), 
               must remember the last acyclic pair to encycle! the result */
            last_apair = last_pair;
            pairs = cpairs;
        } else {
            /* this is the first (maybe only) run of the loop 
               (the acyclic part) */
            pairs = apairs;
        }

        while(pairs--) {
            TValue first = kcar(tail);
            tail = kcdr(tail);

            /* skip over non final nils, but final nil
               should be added as last pair to let the result
               be even */
            if (ttisnil(first)) {
                if (ttisnil(tail)) {
                    kset_cdr(last_pair, kcons(K, first, KNIL));
                }
                continue; 
            }

            TValue ftail = first;
            TValue flastp = first;

            /* find the last pair to check the object */
            while(ttispair(ftail) && !kis_marked(ftail)) {
                kmark(ftail);
                flastp = ftail; /* remember last pair */
                ftail = kcdr(ftail);
            }
	
            /* can't unmark the list till the errors are checked,
               otherwise the unmarking may be incorrect */
            if (ttisnil(tail)) {
                /* last argument has special treatment */
                if (ttispair(ftail) && ttisnil(kcdr(ftail))) {
                    /* repeated last pair, this is the only check
                       that is done on the last argument */
                    appendB_clear_last_pairs(K, last_pairs);
                    unmark_list(K, first);
                    klispE_throw_simple(K, "repeated last pairs");
                    return KINERT;
                } else {
                    unmark_list(K, first);
                    /* add last object to the endpoints list, don't add
                       its last pair */
                    kset_cdr(last_pair, kcons(K, first, KNIL));
                }
            } else { /* non final argument, must be an acyclic list 
                        with unique, mutable last pair */
                if (ttisnil(ftail)) {
                    /* acyclic list with non repeated last pair,
                       check mutability */
                    unmark_list(K, first);
                    if (kis_immutable(flastp)) {
                        appendB_clear_last_pairs(K, last_pairs);
                        klispE_throw_simple(K, "immutable pair found");
                        return KINERT;
                    }
                    /* add the last pair to the list of last pairs */
                    kset_mark(flastp, last_pairs);
                    last_pairs = flastp;
		
                    /* add both the first and last pair to the endpoints 
                       list */
                    TValue new_pair = kcons(K, first, KNIL);
                    kset_cdr(last_pair, new_pair);
                    last_pair = new_pair;
                    new_pair = kcons(K, flastp, KNIL);
                    kset_cdr(last_pair, new_pair);
                    last_pair = new_pair;
                } else {
                    /* impoper list or repeated last pair or cyclic list */
                    appendB_clear_last_pairs(K, last_pairs);
                    unmark_list(K, first);

                    if (ttispair(ftail)) {
                        if (ttisnil(kcdr(ftail))) {
                            klispE_throw_simple(K, "repeated last pairs");
                        } else {
                            klispE_throw_simple(K, "cyclic list as non last "
                                                "argument");
                        }  
                    } else {
                        klispE_throw_simple(K, "improper list as non last "
                                            "argument");
                    }
                    return KINERT;
                }
            }
        }
        if (apairs != 0) {
            /* acyclic part done */
            apairs = 0;
        } else {
            /* cyclic part done, program encycle if necessary */
            cpairs = 0;
            if (!tv_equal(last_apair, last_pair)) {
                TValue first_cpair = kcadr(last_apair);
                kset_cdr(last_pair, kcons(K, first_cpair, KNIL));
            } else {
                /* all elements of the cycle are (), add extra
                   nil to simplify the code setting the cdrs */
                kset_cdr(last_pair, kcons(K, KNIL, KNIL));
            }
        }
    }

    appendB_clear_last_pairs(K, last_pairs);

    /* discard the first element (there is always one) because it
       isn't necessary, the list is used to set the last pairs of
       the objects to the correspoding next first pair */
    krooted_vars_pop(K);
    return kcdr(kcdr(elist));
}

/* 6.4.1 append! */
void appendB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    if (ttisnil(ptree)) {
        klispE_throw_simple(K, "no lists");
        return;
    } else if (!ttispair(ptree)) {
        klispE_throw_simple(K, "bad ptree");
        return;
    } else if (ttisnil(kcar(ptree))) {
        klispE_throw_simple(K, "empty first list");
        return;
    }
    TValue lss = ptree;
    TValue first_ls = kcar(lss);
    int32_t pairs, cpairs;
    /* ASK John: if encycle! has only one argument, can't it be cyclic? 
       the report says no, but the wording is poor */
    check_list(K, false, first_ls, NULL, NULL);
    check_list(K, true, lss, &pairs, &cpairs);
    int32_t apairs = pairs - cpairs;

    TValue endpoints = 
        appendB_get_lss_endpoints(K, lss, apairs, cpairs);
    /* connect all the last pairs to the corresponding next first pair,
       endpoints is even */
    while(!ttisnil(endpoints)) {
        TValue first = kcar(endpoints);
        endpoints = kcdr(endpoints);
        TValue second = kcar(endpoints);
        endpoints = kcdr(endpoints);
        kset_cdr(first, second);
    }
    kapply_cc(K, KINERT);
}

/* 6.4.2 copy-es */
/* uses copy_es helper (above copy-es-immutable) */

/* 6.4.3 assq */
/* REFACTOR: do just one pass, maybe use generalized accum function */
void assq(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_2p(K, ptree, obj, ls);
    /* first pass, check structure */
    int32_t pairs;
    check_typed_list(K, kpairp, true, ls, &pairs, NULL);
    TValue tail = ls;
    TValue res = KNIL;
    while(pairs--) {
        TValue first = kcar(tail);
        if (eq2p(K, kcar(first), obj)) {
            res = first;
            break;
        }
        tail = kcdr(tail);
    }

    kapply_cc(K, res);
}

/* 6.4.3 memq? */
/* REFACTOR: do just one pass, maybe use generalized accum function */
void memqp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_2p(K, ptree, obj, ls);
    /* first pass, check structure */
    int32_t pairs;
    check_list(K, true, ls, &pairs, NULL);
    TValue tail = ls;
    TValue res = KFALSE;
    while(pairs--) {
        TValue first = kcar(tail);
        if (eq2p(K, first, obj)) {
            res = KTRUE;
            break;
        }
        tail = kcdr(tail);
    }

    kapply_cc(K, res);
}

/* ?.? immutable-pair?, mutable-pair */
/* use ftypep */

/* init ground */
void kinit_pair_mut_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 4.7.1 set-car!, set-cdr! */
    add_applicative(K, ground_env, "set-car!", set_carB, 0);
    add_applicative(K, ground_env, "set-cdr!", set_cdrB, 0);
    /* 4.7.2 copy-es-immutable */
    add_applicative(K, ground_env, "copy-es-immutable", copy_es, 2, symbol, 
                    b2tv(false));
    /* 5.8.1 encycle! */
    add_applicative(K, ground_env, "encycle!", encycleB, 0);
    /* 6.?? list-set! */
    add_applicative(K, ground_env, "list-set!", list_setB, 0);
    /* 6.4.1 append! */
    add_applicative(K, ground_env, "append!", appendB, 0);
    /* 6.4.2 copy-es */
    add_applicative(K, ground_env, "copy-es", copy_es, 2, symbol, b2tv(true));
    /* 6.4.3 assq */
    add_applicative(K, ground_env, "assq", assq, 0);
    /* 6.4.3 memq? */
    add_applicative(K, ground_env, "memq?", memqp, 0);
    /* ?.? immutable-pair?, mutable-pair? */
    add_applicative(K, ground_env, "immutable-pair?", ftypep, 2, symbol, 
                    p2tv(kimmutable_pairp));
    add_applicative(K, ground_env, "mutable-pair?", ftypep, 2, symbol, 
                    p2tv(kmutable_pairp));
}
