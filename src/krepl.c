/*
** krepl.c
** klisp repl
** See Copyright Notice in klisp.h
*/
#include <stdio.h>
#include <setjmp.h>

#include "klisp.h"
#include "kstate.h"
#include "kobject.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kwrite.h"
#include "kread.h"
#include "krepl.h"

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
