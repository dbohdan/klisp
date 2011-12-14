/*
** kgbooleans.h
** Boolean features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kpair.h"
#include "ksymbol.h"
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgbooleans.h"

/* Continuations */
void do_Sandp_Sorp(klisp_State *K);

/* 4.1.1 boolean? */
/* uses typep */

/* 6.1.1 not? */
void notp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "boolean", ttisboolean, tv_b);

    TValue res = kis_true(tv_b)? KFALSE : KTRUE;
    kapply_cc(K, res);
}

/* 6.1.2 and? */
void andp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    int32_t pairs;
    /* don't care about cycle pairs */
    check_typed_list(K, kbooleanp, true, ptree, &pairs, NULL);
    TValue res = KTRUE;
    TValue tail = ptree;
    while(pairs--) {
        TValue first = kcar(tail);
        tail = kcdr(tail);
        if (kis_false(first)) {
            res = KFALSE;
            break;
        }
    }
    kapply_cc(K, res);
}

/* 6.1.3 or? */
void orp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    int32_t pairs; 
    /* don't care about cycle pairs */
    check_typed_list(K, kbooleanp,true, ptree, &pairs, NULL);
    TValue res = KFALSE;
    TValue tail = ptree;
    while(pairs--) {
        TValue first = kcar(tail);
        tail = kcdr(tail);
        if (kis_true(first)) {
            res = KTRUE;
            break;
        }
    }
    kapply_cc(K, res);
}

/* Helpers for $and? & $or? */

/*
** operands is a list, the other cases are handled before calling 
** term-bool is the termination boolean, i.e. the boolean that terminates 
** evaluation early and becomes the result of $and?/$or?
** it is #t for $or? and #f for $and?
** both $and? & $or? have to allow boolean checking while performing a tail 
** call that is acomplished by checking if the current continuation will 
** perform a boolean check, and in that case, no continuation is created
*/
void do_Sandp_Sorp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: termination boolean
    ** xparams[2]: remaining operands
    ** xparams[3]: denv
    */
    TValue sname = xparams[0];
    TValue term_bool = xparams[1];
    TValue ls = xparams[2];
    TValue denv = xparams[3];

    if (!ttisboolean(obj)) {
        klispE_throw_simple_with_irritants(K, "expected boolean", 1, 
                                           obj);
        return;
    } else if (ttisnil(ls) || tv_equal(obj, term_bool)) {
        /* in both cases the value to be returned is obj:
           if there are no more operands it is obvious otherwise, if
           the termination bool is found:
           $and? returns #f when it finds #f and $or? returns #t when it 
           finds #t */
        kapply_cc(K, obj);
    } else {
        TValue first = kcar(ls);
        TValue tail = kcdr(ls);
        /* This is the important part of tail context + bool check */
        if (!ttisnil(tail) || !kis_bool_check_cont(kget_cc(K))) {
            TValue new_cont = 
                kmake_continuation(K, kget_cc(K), do_Sandp_Sorp, 
                                   4, sname, term_bool, tail, denv);
            /* 
            ** Mark as a bool checking cont this is needed in the last operand
            ** to allow both tail recursive behaviour and boolean checking.
            ** While it is not necessary if this is not the last operand it
            ** avoids a continuation in the last evaluation of the inner form 
            ** in the common use of 
            ** ($and?/$or? ($or?/$and? ...) ...)
            */
            kset_bool_check_cont(new_cont);
            kset_cc(K, new_cont);
#if KTRACK_SI
            /* put the source info of the list including the element
               that we are about to evaluate */
            kset_source_info(K, new_cont, ktry_get_si(K, ls));
#endif
        }
        ktail_eval(K, first, denv);
    }
}

void Sandp_Sorp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: termination boolean
    */
    TValue sname = xparams[0];
    TValue term_bool = xparams[1];
    
    TValue ls = check_copy_list(K, ptree, false, NULL, NULL);
    /* This will work even if ls is empty */
    krooted_tvs_push(K, ls);
    TValue new_cont = kmake_continuation(K, kget_cc(K), do_Sandp_Sorp, 4, 
                                         sname, term_bool, ls, denv);
    krooted_tvs_pop(K);
    /* there's no need to mark it as bool checking, no evaluation
       is done in the dynamic extent of this cont, no need for 
       source info either */
    kset_cc(K, new_cont);
    kapply_cc(K, knegp(term_bool)); /* pass dummy value to start */
}

/* 6.1.4 $and? */
/* uses Sandp_Sorp */

/* 6.1.5 $or? */
/* uses Sandp_Sorp */

/* init ground */
void kinit_booleans_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 4.1.1 boolean? */
    add_applicative(K, ground_env, "boolean?", typep, 2, symbol, 
                    i2tv(K_TBOOLEAN));
    /* 6.1.1 not? */
    add_applicative(K, ground_env, "not?", notp, 0);
    /* 6.1.2 and? */
    add_applicative(K, ground_env, "and?", andp, 0);
    /* 6.1.3 or? */
    add_applicative(K, ground_env, "or?", orp, 0);
    /* 6.1.4 $and? */
    add_operative(K, ground_env, "$and?", Sandp_Sorp, 2, symbol, KFALSE);
    /* 6.1.5 $or? */
    add_operative(K, ground_env, "$or?", Sandp_Sorp, 2, symbol, KTRUE);
}

/* init continuation names */
void kinit_booleans_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);
    add_cont_name(K, t, do_Sandp_Sorp, "eval-booleans");
}
