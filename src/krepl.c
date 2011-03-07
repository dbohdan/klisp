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
#include "kenvironment.h"
#include "kerror.h"
#include "kread.h"
#include "kwrite.h"
#include "kstring.h"
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

/* the underlying function of the read cont */
void read_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    (void) obj;
    (void) xparams;

    /* show prompt */
    fprintf(stdout, "klisp> ");
    obj = kread(K);
    kapply_cc(K,obj);
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

void loop_fn(klisp_State *K, TValue *xparams, TValue obj);

/* this is called from both loop_fn and error_fn */
inline void create_loop(klisp_State *K, TValue denv)
{
    TValue loop_cont = kmake_continuation(
	K, K->root_cont, KNIL, KNIL, &loop_fn, 1, denv);
    TValue eval_cont = kmake_continuation(
	K, loop_cont, KNIL, KNIL, &eval_cfn, 1, denv);
    TValue read_cont = kmake_continuation(
	K, eval_cont, KNIL, KNIL, &read_fn, 0);
    kset_cc(K, read_cont);
    kapply_cc(K, KINERT);
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
	create_loop(K, denv);
    }
} 

/* the underlying function of the error cont */
void error_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */
    /* TEMP: obj should be a string */
    /* TODO: create some kind of error object */
    char *str = ttisstring(obj)?
	kstring_buf(obj) : "not a string passed to error continuation";

    fprintf(stderr, "\n*ERRROR*: %s\n", str);

    TValue denv = xparams[0];
    create_loop(K, denv);
}

/* call this to init the repl in a newly created klisp state */
void kinit_repl(klisp_State *K)
{
    TValue std_env = kmake_environment(K, K->ground_env);

    /* set up the continuations */
    TValue root_cont = kmake_continuation(K, KNIL, KNIL, KNIL,
					  exit_fn, 0);
    TValue error_cont = kmake_continuation(K, KNIL, KNIL, KNIL,
					   error_fn, 1, std_env);

    K->root_cont = root_cont;
    K->error_cont = error_cont;

    create_loop(K, std_env);
}
