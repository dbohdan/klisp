/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <setjmp.h>

/* turn on assertions for internal checking */
#define klisp_assert (assert)
#include "klimits.h"

#include "klisp.h"
#include "kobject.h"
#include "kauxlib.h"
#include "kstate.h"
#include "kread.h"
#include "kwrite.h"
#include "keval.h"
#include "krepl.h"

#include "kcontinuation.h"
#include "kenvironment.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kpair.h"
#include "ksymbol.h"
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
	klispE_throw(K, "Bad syntax ($define!)", true);
	return;
    }
    TValue ptree = kcar(obj);
    TValue exp = kcar(kcdr(obj));
    /* TODO: allow general ptrees */
    if (!ttissymbol(ptree) && !ttisignore(ptree)) {
	klispE_throw(K, "Not a symbol or ignore ($define!)", true);
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
	klispE_throw(K, "Bad syntax (cons)", true);
	return;
    }
    TValue car = kcar(obj);
    TValue cdr = kcar(kcdr(obj));
    TValue new_pair = kcons(K, car, cdr);
    kapply_cc(K, new_pair);
}

int main(int argc, char *argv[]) 
{
    printf("Read/Write Test\n");

    klisp_State *K = klispL_newstate();

    /* set up the continuations */
    K->eval_op = kmake_operative(K, KNIL, KNIL, keval_ofn, 0);
    TValue ground_env = kmake_empty_environment(K);

    TValue g_define = kmake_operative(K, KNIL, KNIL, def_ofn, 0);
    TValue s_define = ksymbol_new(K, "$define!");
    kadd_binding(K, ground_env, s_define, g_define);

    TValue g_cons = kwrap(K, kmake_operative(K, KNIL, KNIL, cons_ofn, 0));
    TValue s_cons = ksymbol_new(K, "cons");
    kadd_binding(K, ground_env, s_cons, g_cons);

    TValue std_env = kmake_environment(K, ground_env);
    TValue root_cont = kmake_continuation(K, KNIL, KNIL, KNIL,
					  exit_fn, 0);
    TValue loop_cont = kmake_continuation(
	K, root_cont, KNIL, KNIL, &loop_fn, 1, std_env);
    TValue eval_cont = kmake_continuation(
	K, loop_cont, KNIL, KNIL, &eval_cfn, 1, std_env);
    
    kset_cc(K, eval_cont);
    /* NOTE: this will take effect only in the while (K->next_func) loop */
    klispS_apply_cc(K, kread(K));

    int ret_value = 0;
    bool done = false;

    while(!done) {
	if (setjmp(K->error_jb)) {
	    /* error signaled */
	    if (K->error_can_cont) {
		/* XXX: clear stack and char buffer, clear shared dict */
		/* TODO: put these in handlers for read-token, read and write */
		ks_sclear(K);
		ks_tbclear(K);
		K->shared_dict = KNIL;
		
		kset_cc(K, eval_cont);
		/* NOTE: this will take effect only in the while (K->next_func) loop */
		klispS_apply_cc(K, kread(K));
	    } else {
		printf("Aborting...\n");
		ret_value = 1;
		done = true;
	    }
	} else {
	    /* all ok, continue with next func */
	    while (K->next_func) {
		if (ttisnil(K->next_env)) {
		    /* continuation application */
		    klisp_Cfunc fn = (klisp_Cfunc) K->next_func;
		    (*fn)(K, K->next_xparams, K->next_value);
		} else {
		    /* operative calling */
		    klisp_Ofunc fn = (klisp_Ofunc) K->next_func;
		    (*fn)(K, K->next_xparams, K->next_value, K->next_env);
		}
	    }
	    printf("Done!\n");
	    ret_value = 0;
	    done = true;
	}
    }

    klisp_close(K);
    return ret_value;
}
