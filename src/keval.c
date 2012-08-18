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
void do_eval_ls(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: remaining list
    ** xparams[1]: rem_pairs
    ** xparams[2]: accumulated list
    ** xparams[3]: dynamic environment
    ** xparams[4]: apairs
    ** xparams[5]: cpairs
    */
    TValue rest = xparams[0];
    int32_t rem_pairs = ivalue(xparams[1]);
    TValue acc = xparams[2];
    TValue env = xparams[3];
    TValue tv_apairs = xparams[4];
    TValue tv_cpairs = xparams[5];

    acc = kcons(K, obj, acc);
    krooted_tvs_push(K, acc);

    if (rem_pairs == 0) {
        /* argument evaluation complete, copy the list and encycle if 
         needed (the list was reversed during evaluation, so it should
         be reversed first) */
        TValue res = 
	  reverse_copy_and_encycle(K, acc, ivalue(tv_apairs) + 
				   ivalue(tv_cpairs), ivalue(tv_cpairs));
        krooted_tvs_pop(K); /* pop acc */
        kapply_cc(K, res);
    } else {
        /* more arguments need to be evaluated */
        /* GC: all objects are rooted at this point */
        TValue new_cont = 
            kmake_continuation(K, kget_cc(K), do_eval_ls, 6, kcdr(rest), 
                               i2tv(rem_pairs - 1), acc, env, tv_apairs, 
			       tv_cpairs);
        krooted_tvs_pop(K); /* pop acc */
        kset_cc(K, new_cont);
        ktail_eval(K, kcar(rest), env);
    }
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
	  int32_t pairs, apairs, cpairs;
            TValue comb_cont = 
                kmake_continuation(K, kget_cc(K), do_combine_operator, 
                                   3, tv2app(comb)->underlying, env, si);

            krooted_tvs_push(K, comb_cont);
            /* list is copied reversed to eval right to left and
               avoid mutation of the structure affecting evaluation;
               this also allows capturing continuations in the middle of
               argument evaluation with no additional overhead */
            TValue arg_ls = check_copy_list(K, operands, false, 
					    &pairs, &cpairs);
	    apairs = pairs - cpairs;
            krooted_tvs_push(K, arg_ls);
            TValue els_cont = 
                kmake_continuation(K, comb_cont, do_eval_ls, 6, kcdr(arg_ls), 
				   i2tv(pairs - 1), KNIL, env, i2tv(apairs), 
				   i2tv(cpairs));
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

