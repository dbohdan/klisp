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

/* the underlying function of the eval cont */
void eval_ofn(klisp_State *K, TValue *xparams, TValue obj, TValue env)
{
    (void) xparams;
    (void) env;

    switch(ttype(obj)) {
    case K_TPAIR:
	/* TODO */
	kapply_cc(K, obj);
	break;
    case K_TSYMBOL:
	/* TODO */
	kapply_cc(K, obj);
	break;
    default:
	kapply_cc(K, obj);
    }
}

/* the underlying function of the eval operative */
void eval_cfn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** tparams[0]: eval operative
    ** tparams[1]: dynamic environment
    */
    TValue eval_op = xparams[0];
    TValue denv = xparams[1];
    
    ktail_call(K, eval_op, obj, denv);
}

/* the underlying function of the write & loop  cont */
void loop_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** tparams[0]: eval operative
    ** tparams[1]: dynamic environment
    */
    if (ttiseof(obj)) {
	/* this will in turn call main_cont */
	kapply_cc(K, obj);
    } else {
	kwrite(K, obj);
	knewline(K);
	TValue eval_op = xparams[0];
	TValue denv = xparams[1];

	TValue loop_cont = kmake_continuation(
	    K, kget_cc(K), KNIL, KNIL, &loop_fn, 2, eval_op, denv);
	TValue eval_cont = kmake_continuation(
	    K, loop_cont, KNIL, KNIL, &eval_cfn, 2, eval_op, denv);
	kset_cc(K, eval_cont);
	TValue robj = kread(K);
	kapply_cc(K, robj);
    }
} 

int main(int argc, char *argv[]) 
{
    printf("Read/Write Test\n");

    klisp_State *K = klispL_newstate();

    /* set up the continuations */
    TValue eval_op = kmake_operative(K, KNIL, KNIL, eval_ofn, 0);
    TValue ground_env = kmake_empty_environment(K);
    TValue std_env = kmake_environment(K, ground_env);
    TValue root_cont = kmake_continuation(K, KNIL, KNIL, KNIL,
					  exit_fn, 0);
    TValue loop_cont = kmake_continuation(
	K, root_cont, KNIL, KNIL, &loop_fn, 2, eval_op, std_env);
    TValue eval_cont = kmake_continuation(
	K, loop_cont, KNIL, KNIL, &eval_cfn, 2, eval_op, std_env);
    
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
