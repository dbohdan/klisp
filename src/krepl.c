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
#include "kgerror.h"
/* for names */
#include "ktable.h"
/* for do_pass_value */
#include "kgcontinuations.h"

/* TODO add names & source info to the repl continuations */

/* the underlying function of the read cont */
void do_repl_read(klisp_State *K, TValue *xparams, TValue obj)
{
    UNUSED(xparams);
    UNUSED(obj);

    /* show prompt */
    fprintf(stdout, KLISP_PROMPT);

    TValue port = kcdr(K->kd_in_port_key);
    klisp_assert(kfport_file(port) == stdin);
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
	/* print a newline to allow the shell a fresh line */
	printf("\n");
	/* This is ok because there is no interception possible */
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
void do_int_repl_error(klisp_State *K, TValue *xparams, TValue ptree,
		       TValue denv);

/* this is called from both do_repl_loop and do_repl_error */
/* GC: assumes denv is NOT rooted */
void create_loop(klisp_State *K, TValue denv)
{
    krooted_tvs_push(K, denv);

    /* TODO this should be factored out, it is quite common */
    TValue error_int = kmake_operative(K, do_int_repl_error, 1, denv);
    krooted_tvs_pop(K); /* already in cont */
    krooted_tvs_push(K, error_int);
    TValue exit_guard = kcons(K, K->error_cont, error_int);
    krooted_tvs_pop(K); /* already in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* already in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_guards = KNIL;

    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, K->root_cont, 
					   do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
					   do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    /* stack is empty now */
    krooted_tvs_push(K, inner_cont);

    TValue loop_cont = 
	kmake_continuation(K, inner_cont, do_repl_loop, 1, denv);
    krooted_tvs_pop(K); /* in loop cont */
    krooted_tvs_push(K, loop_cont);
    TValue eval_cont = kmake_continuation(K, loop_cont, do_repl_eval, 1, denv);
    krooted_tvs_pop(K); /* in eval cont */
    krooted_tvs_push(K, eval_cont);
    TValue read_cont = kmake_continuation(K, eval_cont, do_repl_read, 0);
    kset_cc(K, read_cont);
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
    klisp_assert(kfport_file(port) == stdout);

    /* false: quote strings, escape chars */
    kwrite_display_to_port(K, port, obj, false);
    kwrite_newline_to_port(K, port);

    TValue denv = xparams[0];
    create_loop(K, denv);
} 

/* the underlying function of the error cont */
void do_int_repl_error(klisp_State *K, TValue *xparams, TValue ptree,
    TValue denv)
{
    /* 
    ** xparams[0]: dynamic environment
    */

    UNUSED(denv);

    /*
    ** ptree is (object divert)
    */
    TValue obj = kcar(ptree);
    TValue divert = kcadr(ptree);

    /* FOR NOW used only for irritant list */
    TValue port = kcdr(K->kd_error_port_key);
    klisp_assert(ttisfport(port) && kfport_file(port) == stderr);

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
	fprintf(stderr, "\n*ERROR*: \n");
	fprintf(stderr, "%s: %s", who_str, msg);

	krooted_tvs_push(K, obj);

	/* Msg + irritants */
	/* TODO move to a new function */
	if (!ttisnil(err_obj->irritants)) {
	    fprintf(stderr, ": ");
	    kwrite_display_to_port(K, port, err_obj->irritants, false);
	}
	kwrite_newline_to_port(K, port);

#if KTRACK_NAMES
#if KTRACK_SI
	/* Location */
	/* TODO move to a new function */
	/* MAYBE: remove */
	if (khas_name(who) || khas_si(who)) {
	    fprintf(stderr, "Location: ");
	    kwrite_display_to_port(K, port, who, false);
	    kwrite_newline_to_port(K, port);
	}

	/* Backtrace */
	/* TODO move to a new function */
	TValue tv_cont = err_obj->cont;
	fprintf(stderr, "Backtrace: \n");
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
	fprintf(stderr, "\n*ERROR*: not an error object passed to " 
		"error continuation");
    }

    UNUSED(divert);
    TValue old_denv = xparams[0];
    /* this is the same as a divert */
    create_loop(K, old_denv);
}

/* call this to init the repl in a newly created klisp state */
/* the standard environment should be in K->next_env */
void kinit_repl(klisp_State *K)
{
    TValue std_env = K->next_env;

    #if KTRACK_SI
    /* save the root cont in next_si to let the loop continuations have 
       source info, this is hackish but works */
    
    K->next_si = ktry_get_si(K, K->root_cont);
    #endif

    /* GC: create_loop will root std_env */
    create_loop(K, std_env);
}
