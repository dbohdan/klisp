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

#include "kcontinuation.h"
#include "kenvironment.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kpair.h"
#include "ksymbol.h"
#include "kerror.h"

/* the exit continuation, it exits the loop */
void exit_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* avoid warnings */
    (void) xparams;
    (void) obj;

    /* force the loop to terminate */
    K->next_func = NULL;
    return;
}

/* eval helpers */
void eval_ls_cfn(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: this argument list pair
    ** xparams[1]: dynamic environment
    ** xparams[2]: first-cycle-pair/NIL 
    ** xparams[3]: combiner
    */
    TValue apair = xparams[0];
    TValue rest = kcdr(apair);
    TValue env = xparams[1];
    TValue tail = xparams[2];
    TValue combiner = xparams[3];

    /* save the result of last evaluation and continue with next pair */
    kset_car(apair, obj); 
    if (ttisnil(rest)) {
	/* argument evaluation complete */
	/* this is necessary to recreate the cycle in operand list */
	kset_cdr(apair, tail);
	kapply_cc(K, combiner);
    } else {
	/* more arguments need to be evaluated */
	TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL, 
					     &eval_ls_cfn, 4, rest, env, 
					     tail, combiner);
	kset_cc(K, new_cont);
	ktail_call(K, K->eval_op, kcar(rest), env);
    }
}

inline void clear_ls_marks(TValue ls)
{
    while (ttispair(ls) && kis_marked(ls)) {
	kunmark(ls);
	ls = kcdr(ls);
    }
}

/* operands should be a pair */
inline TValue make_arg_ls(klisp_State *K, TValue operands, TValue *tail)
{
    TValue arg_ls = kcons(K, kcar(operands), KNIL);
    TValue last_pair = arg_ls;
    kset_mark(operands, last_pair);
    TValue rem_op = kcdr(operands);
    
    while(ttispair(rem_op) && kis_unmarked(rem_op)) {
	TValue new_pair = kcons(K, kcar(rem_op), KNIL);
	kset_mark(rem_op, new_pair);
	kset_cdr(last_pair, new_pair);
	last_pair = new_pair;
	rem_op = kcdr(rem_op);
    }
    
    if (ttispair(rem_op)) {
	/* cyclical list */
	*tail = kget_mark(rem_op);
    } else if (ttisnil(rem_op)) {
	*tail = KNIL;
    } else {
	clear_ls_marks(operands);
	klispE_throw(K, "Not a list in applicative combination", true);
	return KINERT;
    }
    clear_ls_marks(operands);
    return arg_ls;
}

void combine_cfn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: operand list
    ** xparams[1]: dynamic environment
    */
    TValue operands = xparams[0];
    TValue env = xparams[1];

    switch(ttype(obj)) {
    case K_TAPPLICATIVE: {
	if (ttisnil(operands)) {
	    /* no arguments => no evaluation, just call the operative */
	    /* NOTE: the while is needed because it may be multiply wrapped */
	    while(ttisapplicative(obj))
		obj = tv2app(obj)->underlying;
	    ktail_call(K, obj, operands, env);
	} else if (ttispair(operands)) {
	    /* make a copy of the operands (for storing arguments) */
	    TValue tail;
	    TValue arg_ls = make_arg_ls(K, operands, &tail);

	    TValue comb_cont = kmake_continuation(
		K, kget_cc(K), KNIL, KNIL, &combine_cfn, 2, arg_ls, env);

	    TValue els_cont = kmake_continuation(
		K, comb_cont, KNIL, KNIL, &eval_ls_cfn, 
		4, arg_ls, env, tail, tv2app(obj)->underlying);
	    kset_cc(K, els_cont);
	    ktail_call(K, K->eval_op, kcar(arg_ls), env);
	}
    }
    case K_TOPERATIVE:
	ktail_call(K, obj, operands, env);
    default:
	klispE_throw(K, "Not a combiner in combiner position", true);
	return;
    }
}

/* the underlying function of the eval cont */
void eval_ofn(klisp_State *K, TValue *xparams, TValue obj, TValue env)
{
    (void) xparams;

    switch(ttype(obj)) {
    case K_TPAIR: {
	TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     &combine_cfn, 2, kcdr(obj), env);
	kset_cc(K, new_cont);
	ktail_call(K, K->eval_op, kcar(obj), env);
	break;
    }
    case K_TSYMBOL:
	/* error handling happens in kget_binding */
	kapply_cc(K, kget_binding(K, env, obj));
	break;
    default:
	kapply_cc(K, obj);
    }
}

/* the underlying function of the eval operative */
void eval_cfn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */
    TValue denv = xparams[0];
    
    ktail_call(K, K->eval_op, obj, denv);
}

/* the underlying function of the write & loop  cont */
void loop_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */
    if (ttiseof(obj)) {
	/* this will in turn call main_cont */
	kapply_cc(K, obj);
    } else {
	kwrite(K, obj);
	knewline(K);
	TValue denv = xparams[0];

	TValue loop_cont = kmake_continuation(
	    K, kget_cc(K), KNIL, KNIL, &loop_fn, 1, denv);
	TValue eval_cont = kmake_continuation(
	    K, loop_cont, KNIL, KNIL, &eval_cfn, 1, denv);
	kset_cc(K, eval_cont);
	TValue robj = kread(K);
	kapply_cc(K, robj);
    }
} 

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
    K->eval_op = kmake_operative(K, KNIL, KNIL, eval_ofn, 0);
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
