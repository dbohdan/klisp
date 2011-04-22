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
#include "ksymbol.h"
#include "kport.h"
#include "kpair.h"

/* the exit continuation, it exits the loop */
void exit_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    UNUSED(xparams);
    UNUSED(obj);

    /* force the loop to terminate */
    K->next_func = NULL;
    return;
}

/* the underlying function of the read cont */
void read_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    UNUSED(xparams);
    UNUSED(obj);

    /* show prompt */
    fprintf(stdout, "klisp> ");

    TValue port = kcdr(K->kd_in_port_key);
    klisp_assert(kport_file(port) == stdin);

    kport_reset_source_info(port);
    obj = kread_from_port(K, port, true); /* read mutable pairs */
    kapply_cc(K, obj);
}

/* the underlying function of the eval cont */
void eval_cfn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */
    TValue denv = xparams[0];
    
    if (ttiseof(obj)) {
	/* read [EOF], should terminate the repl */
	/* this will in turn call main_cont */
	kset_cc(K, K->root_cont);
	kapply_cc(K, KINERT);
    } else {
	ktail_eval(K, obj, denv);
    }
}

void loop_fn(klisp_State *K, TValue *xparams, TValue obj);

/* this is called from both loop_fn and error_fn */
/* GC: assumes denv is NOT rooted */
inline void create_loop(klisp_State *K, TValue denv)
{
    krooted_tvs_push(K, denv);
    TValue loop_cont = 
	kmake_continuation(K, K->root_cont, &loop_fn, 1, denv);
    krooted_tvs_push(K, loop_cont);
    TValue eval_cont = kmake_continuation(K, loop_cont, &eval_cfn, 1, denv);
    krooted_tvs_pop(K); /* in eval cont */
    krooted_tvs_push(K, eval_cont);
    TValue read_cont = kmake_continuation(K, eval_cont, &read_fn, 0);
    kset_cc(K, read_cont);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* the underlying function of the write & loop  cont */
void loop_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */

    TValue port = kcdr(K->kd_out_port_key);
    klisp_assert(kport_file(port) == stdout);

    /* false: quote strings, escape chars */
    kwrite_display_to_port(K, port, obj, false);
    kwrite_newline_to_port(K, port);

    TValue denv = xparams[0];
    create_loop(K, denv);
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

    fprintf(stderr, "\n*ERROR*: %s\n", str);

    TValue denv = xparams[0];
    create_loop(K, denv);
}

/* call this to init the repl in a newly created klisp state */
void kinit_repl(klisp_State *K)
{
    TValue std_env = kmake_environment(K, K->ground_env);
    krooted_tvs_push(K, std_env);

    /* set up the continuations */
    TValue root_cont = kmake_continuation(K, KNIL, exit_fn, 0);
    krooted_tvs_push(K, root_cont);

    TValue error_cont = kmake_continuation(K, root_cont, error_fn, 1, std_env);
    krooted_tvs_push(K, error_cont);

    /* update the ground environment with these two conts */
    TValue symbol;
    symbol = ksymbol_new(K, "root-continuation");
    /* GC: symbol should already be in root */
    kadd_binding(K, K->ground_env, symbol, root_cont);
    symbol = ksymbol_new(K, "error-continuation"); 
    /* GC: symbol should already be in root */
    kadd_binding(K, K->ground_env, symbol, error_cont);

    /* and save them in the structure */
    K->root_cont = root_cont;
    K->error_cont = error_cont;

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    /* GC: create_loop will root std_env */
    create_loop(K, std_env);
}
