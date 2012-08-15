/*
** keval.c
** klisp eval function
** See Copyright Notice in klisp.h
*/

#include "klisp.h"
#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kenvironment.h"
#include "kcontinuation.h"
#include "kerror.h"

/* for continuation name setting */
#include "kghelpers.h"

/* Continuations */
void do_eval_ls(klisp_State *K);
void do_combine_operator(klisp_State *K);
void do_combine_operands(klisp_State *K);


/*
** Eval helpers 
*/

TValue copy_and_encycle(klisp_State *K, TValue ls, 
                        int32_t apairs, int32_t cpairs)
{
    /* apairs + cpairs > 0 */
    TValue first = kcons(K, kcar(ls), KNIL);
    TValue last = first;
    krooted_tvs_push(K, first);
    ls = kcdr(ls);
    bool has_apairs = apairs > 0;

    if (has_apairs) {
        --apairs;
        while (apairs > 0) {
            TValue np = kcons(K, kcar(ls), KNIL);
            kset_cdr(last, np);
            last = np;
            
            ls = kcdr(ls);
            --apairs;
        }
    }

    if (cpairs > 0) {
        TValue first_c;
        if (has_apairs) {
            first_c = kcons(K, kcar(ls), KNIL);
            kset_cdr(last, first_c);
            last = first_c;
            ls = kcdr(ls);
            --cpairs;
        } else {
            first_c = first; /* also == to last */
            --cpairs; /* cdr was already done above */
        }

        while (cpairs > 0) {
            TValue np = kcons(K, kcar(ls), KNIL);
            kset_cdr(last, np);
            last = np;
            
            ls = kcdr(ls);
            --cpairs;
        }
        kset_cdr(last, first_c);
    }

    krooted_tvs_pop(K);
    return first;
}

void do_eval_ls(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: remaining list
    ** xparams[1]: accumulated list
    ** xparams[2]: dynamic environment
    ** xparams[3]: apairs
    ** xparams[4]: cpairs
    */
    TValue rest = xparams[0];
    TValue acc = xparams[1];
    TValue env = xparams[2];
    TValue tv_apairs = xparams[3];
    TValue tv_cpairs = xparams[4];

    acc = kcons(K, obj, acc);
    krooted_tvs_push(K, acc);

    if (ttisnil(rest)) {
        /* argument evaluation complete, copy the list and encycle if 
         needed (the list was reversed again during evaluation, so it
         is now in the correct order */
        TValue res = copy_and_encycle(K, acc, ivalue(tv_apairs), 
                                      ivalue(tv_cpairs));
        krooted_tvs_pop(K); /* pop acc */
        kapply_cc(K, res);
    } else {
        /* more arguments need to be evaluated */
        /* GC: all objects are rooted at this point */
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_eval_ls, 5, kcdr(rest), 
                               acc, env, tv_apairs, tv_cpairs);
        krooted_tvs_pop(K); /* pop acc */
        kset_cc(K, new_cont);
        ktail_eval(K, kcar(rest), env);
    }
}

/* TODO: move this to another file, to use it elsewhere */
static inline void clear_ls_marks(TValue ls)
{
    while (ttispair(ls) && kis_marked(ls)) {
        kunmark(ls);
        ls = kcdr(ls);
    }
}


/* operands should be a pair, and should be rooted (GC) */
TValue check_reverse_copy_list(klisp_State *K, TValue operands, 
                               int32_t *apairs, int32_t *cpairs)
{
    TValue ls = KNIL;
    TValue rem_op = operands;
    int32_t p = 0;
    int32_t c = 0;

    krooted_tvs_push(K, ls); /* put in stack to maintain while invariant */

    while(ttispair(rem_op) && kis_unmarked(rem_op)) {
        kset_mark(rem_op, i2tv(p)); /* remember index */
        ls = kcons(K, kcar(rem_op), ls);
        krooted_tvs_pop(K);
        krooted_tvs_push(K, ls);
        rem_op = kcdr(rem_op);
        ++p;
    }

    krooted_tvs_pop(K);

    if (ttispair(rem_op)) { /* cyclic list */
        c = p - ivalue(kget_mark(rem_op));
    } else if (ttisnil(rem_op)) { /* regular list */
        /* do nothing */
    } else { /* acyclic list - error */
        clear_ls_marks(operands);
        klispE_throw_simple(K, "Not a list in applicative combination");
        return KINERT;
    }
    clear_ls_marks(operands);

    if (apairs != NULL)
        *apairs = p - c;
    if (cpairs != NULL)
        *cpairs = c;

    return ls;
}

void do_combine_operands(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue comb = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: operand list
    ** xparams[1]: dynamic environment
    ** xparams[2]: original_obj_with_si
    */
    TValue operands = xparams[0];
    TValue env = xparams[1];
    TValue si = xparams[2];

    switch(ttype(comb)) {
    case K_TAPPLICATIVE: {
        if (ttisnil(operands)) {
            /* no arguments => no evaluation, just call the operative */
            /* NOTE: the while is needed because it may be multiply wrapped */
            while(ttisapplicative(comb))
                comb = tv2app(comb)->underlying;
            ktail_call_si(K, comb, operands, env, si);
        } else if (ttispair(operands)) {
            int32_t apairs, cpairs;
            TValue comb_cont = 
                kmake_continuation(K, kget_cc(K), do_combine_operator, 
                                   3, tv2app(comb)->underlying, env, si);

            krooted_tvs_push(K, comb_cont);
            /* list is copied reversed to eval right to left and
               avoid mutation of the structure affecting evaluation;
               this also allows capturing continuations in the middle of
               argument evaluation with no additional overhead */
            TValue arg_ls = check_reverse_copy_list(K, operands, 
                                                    &apairs, &cpairs);
            krooted_tvs_push(K, arg_ls);
            TValue els_cont = 
                kmake_continuation(K, comb_cont, do_eval_ls, 5, kcdr(arg_ls), 
                                   KNIL, env, i2tv(apairs), i2tv(cpairs));
            krooted_tvs_pop(K);
            krooted_tvs_pop(K);

            kset_cc(K, els_cont);
            ktail_eval(K, kcar(arg_ls), env);
        } else {
            klispE_throw_simple(K, "Not a list in applicative combination");
            return;
        }
    }
    case K_TOPERATIVE:
        ktail_call_si(K, comb, operands, env, si);
        break;
    default:
        klispE_throw_simple(K, "Not a combiner in combiner position");
        return;
    }
}

void do_combine_operator(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue arguments = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: combiner
    ** xparams[1]: dynamic environment
    ** xparams[2]: original_obj_with_si
    */
    TValue comb = xparams[0];
    TValue env = xparams[1];
    TValue si = xparams[2];

    switch(ttype(comb)) {
    case K_TAPPLICATIVE: {
        /* we already know arguments is a list, and we already
           have a fresh copy, but we need to reverse it anyway,
           this could be optimized but this case (multiply wrapped
           applicatives) is pretty rare
        */
        break;
    }
    case K_TOPERATIVE:
        ktail_call_si(K, comb, arguments, env, si);
     default: /* this can't really happen */
        klispE_throw_simple(K, "Not a combiner in combiner position");
        return;
    }
}

/* the underlying function of the eval operative */
void keval_ofn(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(xparams);

    TValue obj = ptree;

    switch(ttype(obj)) {
    case K_TPAIR: {
        TValue new_cont = 
             kmake_continuation(K, kget_cc(K), do_combine_operands, 3, 
                                kcdr(obj), denv, ktry_get_si(K, obj));
        kset_cc(K, new_cont);
        ktail_eval(K, kcar(obj), denv);
        break;
    }
    case K_TSYMBOL:
        /* error handling happens in kget_binding */
        kapply_cc(K, kget_binding(K, denv, obj));
        break;
    default:
        kapply_cc(K, obj);
    }
}

/* init continuation names */
void kinit_eval_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);
    add_cont_name(K, t, do_eval_ls, "eval-argument-list");
    add_cont_name(K, t, do_combine_operator, "eval-combine-operator");
    add_cont_name(K, t, do_combine_operands, "eval-combine-operands");
}

