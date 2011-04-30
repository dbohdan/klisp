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
/* for names */
#include "ktable.h"

/* TODO add names & source info to the repl continuations */

/* the exit continuation, it exits the loop */
void do_repl_exit(klisp_State *K, TValue *xparams, TValue obj)
{
    UNUSED(xparams);
    UNUSED(obj);

    /* force the loop to terminate */
    K->next_func = NULL;
    return;
}

/* the underlying function of the read cont */
void do_repl_read(klisp_State *K, TValue *xparams, TValue obj)
{
    UNUSED(xparams);
    UNUSED(obj);

    /* show prompt */
    fprintf(stdout, "klisp> ");

    TValue port = kcdr(K->kd_in_port_key);
    klisp_assert(kport_file(port) == stdin);
#if 0 /* Let's disable this for now */
    /* workaround to the problem of the dangling '\n' in repl 
       (from previous line) */
    kread_ignore_whitespace_and_comments_from_port(K, port);
    
    kport_reset_source_info(port);
#endif
    obj = kread_from_port(K, port, true); /* read mutable pairs */
    kapply_cc(K, obj);
}

/* the underlying function of the eval cont */
void do_repl_eval(klisp_State *K, TValue *xparams, TValue obj)
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
	/* save the source code info of the object in loop_cont
	   before evaling */
#if KTRACK_SI
	kset_source_info(K, kget_cc(K), ktry_get_si(K, obj));
#endif

	ktail_eval(K, obj, denv);
    }
}

void do_repl_loop(klisp_State *K, TValue *xparams, TValue obj);

/* this is called from both do_repl_loop and do_repl_error */
/* GC: assumes denv is NOT rooted */
inline void create_loop(klisp_State *K, TValue denv)
{
    krooted_tvs_push(K, denv);
    TValue loop_cont = 
	kmake_continuation(K, K->root_cont, do_repl_loop, 1, denv);
    krooted_tvs_push(K, loop_cont);
    TValue eval_cont = kmake_continuation(K, loop_cont, do_repl_eval, 1, denv);
    krooted_tvs_pop(K); /* in eval cont */
    krooted_tvs_push(K, eval_cont);
    TValue read_cont = kmake_continuation(K, eval_cont, do_repl_read, 0);
    kset_cc(K, read_cont);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, KINERT);
}

/* the underlying function of the write & loop  cont */
void do_repl_loop(klisp_State *K, TValue *xparams, TValue obj)
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
void do_repl_error(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: dynamic environment
    */

    /* TEMP: should be better to have an error port
       like in scheme r6rs & r7rs (draft) */
    /* FOR NOW used only for irritant list */
    TValue port = kcdr(K->kd_out_port_key);
    klisp_assert(kport_file(port) == stdout);

    /* TEMP: obj should be an error obj */
    if (ttiserror(obj)) {
	Error *err_obj = tv2error(obj);
	TValue who = err_obj->who;
	char *who_str;
	/* TEMP? */
	if (ttiscontinuation(who))
	    who = tv2cont(who)->comb;

	if (ttisstring(who)) {
	    who_str = kstring_buf(who);
#if KTRACK_NAMES
	} else if (khas_name(who)) {
	    TValue name = kget_name(K, who);
	    who_str = ksymbol_buf(name);
#endif
	} else {
	    who_str = "?";
	}
	char *msg = kstring_buf(err_obj->msg);
	fprintf(stdout, "\n*ERROR*: \n");
	fprintf(stdout, "%s: %s", who_str, msg);

	krooted_tvs_push(K, obj);

	/* Msg + irritants */
	/* TODO move to a new function */
	if (!ttisnil(err_obj->irritants)) {
	    fprintf(stdout, ": ");
	    kwrite_display_to_port(K, port, err_obj->irritants, false);
	}
	kwrite_newline_to_port(K, port);

#if KTRACK_NAMES
#if KTRACK_SI
	/* Location */
	/* TODO move to a new function */
	/* MAYBE: remove */
	if (khas_name(who) || khas_si(who)) {
	    fprintf(stdout, "Location: ");
	    kwrite_display_to_port(K, port, who, false);
	    kwrite_newline_to_port(K, port);
	}

	/* Backtrace */
	/* TODO move to a new function */
	TValue tv_cont = err_obj->cont;
	fprintf(stdout, "Backtrace: \n");
	while(ttiscontinuation(tv_cont)) {
	    kwrite_display_to_port(K, port, tv_cont, false);
	    kwrite_newline_to_port(K, port);
	    Continuation *cont = tv2cont(tv_cont);
	    tv_cont = cont->parent;
	}
	/* add extra newline at the end */
	kwrite_newline_to_port(K, port);
#endif
#endif
	krooted_tvs_pop(K);
    } else {
	fprintf(stdout, "\n*ERROR*: not an error object passed to " 
		"error continuation");
    }

    TValue denv = xparams[0];
    create_loop(K, denv);
}

/* call this to init the repl in a newly created klisp state */
void kinit_repl(klisp_State *K)
{
    TValue std_env = kmake_environment(K, K->ground_env);
    krooted_tvs_push(K, std_env);

    /* set up the continuations */
    TValue root_cont = kmake_continuation(K, KNIL, do_repl_exit, 0);
    krooted_tvs_push(K, root_cont);

    TValue error_cont = kmake_continuation(K, root_cont, do_repl_error, 
					   1, std_env);
    krooted_tvs_push(K, error_cont);

    /* update the ground environment with these two conts */
    TValue symbol;
    /* TODO si */
    symbol = ksymbol_new(K, "root-continuation", KNIL);
    krooted_tvs_push(K, symbol);
    kadd_binding(K, K->ground_env, symbol, root_cont);
    krooted_tvs_pop(K);

    #if KTRACK_SI
    /* TODO: find a cleaner way of doing this..., maybe disable gc */
    /* Add source info to the cont */
    TValue str = kstring_new_b_imm(K, __FILE__);
    krooted_tvs_push(K, str);
    TValue tail = kcons(K, i2tv(__LINE__), i2tv(0));
    krooted_tvs_push(K, tail);
    TValue si = kcons(K, str, tail);
    krooted_tvs_push(K, si);
    kset_source_info(K, root_cont, si);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    #endif

    /* TODO si */
    symbol = ksymbol_new(K, "error-continuation", KNIL); 
    krooted_tvs_push(K, symbol);
    kadd_binding(K, K->ground_env, symbol, error_cont);
    krooted_tvs_pop(K);

    #if KTRACK_SI
    str = kstring_new_b_imm(K, __FILE__);
    krooted_tvs_push(K, str);
    tail = kcons(K, i2tv(__LINE__), i2tv(0));
    krooted_tvs_push(K, tail);
    si = kcons(K, str, tail);
    krooted_tvs_push(K, si);
    kset_source_info(K, error_cont, si);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    #endif

    /* and save them in the structure */
    K->root_cont = root_cont;
    K->error_cont = error_cont;

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    #if KTRACK_SI
    /* save the root cont in next_si to let the loop continuations have 
       source info, this is hackish but works */
    
    K->next_si = ktry_get_si(K, K->root_cont);
    #endif

    /* GC: create_loop will root std_env */
    create_loop(K, std_env);
}
