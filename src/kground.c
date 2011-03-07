/*
** kground.h
** Bindings in the ground environment
** See Copyright Notice in klisp.h
*/

/* TODO: split in different files for each module */
#include "kstate.h"
#include "kobject.h"
#include "kground.h"
#include "kpair.h"
#include "kenvironment.h"
#include "kcontinuation.h"
#include "ksymbol.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kerror.h"

/* define helper */
void match_cfn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** tparams[0]: ptree
    ** tparams[1]: dynamic environment
    */
    TValue ptree = xparams[0];
    TValue env = xparams[1];

    /* TODO: allow general parameter trees */
    if (!ttisignore(ptree)) {
	kadd_binding(K, env, ptree, obj);
    }
    kapply_cc(K, KINERT);
}

/* the underlying function of a simple define */
void def_ofn(klisp_State *K, TValue *xparams, TValue obj, TValue env)
{
    (void) xparams;
    if (!ttispair(obj) || !ttispair(kcdr(obj)) || !ttisnil(kcdr(kcdr(obj)))) {
	klispE_throw(K, "Bad syntax ($define!)");
	return;
    }
    TValue ptree = kcar(obj);
    TValue exp = kcar(kcdr(obj));
    /* TODO: allow general ptrees */
    if (!ttissymbol(ptree) && !ttisignore(ptree)) {
	klispE_throw(K, "Not a symbol or ignore ($define!)");
	return;
    } else {
	TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     &match_cfn, 2, ptree, env);
	kset_cc(K, new_cont);
	ktail_call(K, K->eval_op, exp, env);
    }
}

/* the underlying function of cons */
void cons_ofn(klisp_State *K, TValue *xparams, TValue obj, TValue env)
{
    if (!ttispair(obj) || !ttispair(kcdr(obj)) || !ttisnil(kcdr(kcdr(obj)))) {
	klispE_throw(K, "Bad syntax (cons)");
	return;
    }
    TValue car = kcar(obj);
    TValue cdr = kcar(kcdr(obj));
    TValue new_pair = kcons(K, car, cdr);
    kapply_cc(K, new_pair);
}

TValue kmake_ground_env(klisp_State *K)
{
    TValue ground_env = kmake_empty_environment(K);

    TValue g_define = kmake_operative(K, KNIL, KNIL, def_ofn, 0);
    TValue s_define = ksymbol_new(K, "$define!");
    kadd_binding(K, ground_env, s_define, g_define);

    TValue g_cons = kwrap(K, kmake_operative(K, KNIL, KNIL, cons_ofn, 0));
    TValue s_cons = ksymbol_new(K, "cons");
    kadd_binding(K, ground_env, s_cons, g_cons);

    return ground_env;
}
