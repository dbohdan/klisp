/*
** kgports.h
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kport.h"
#include "kenvironment.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kpair.h"
#include "kerror.h"
#include "ksymbol.h"
#include "ktoken.h"
#include "kread.h"
#include "kwrite.h"
#include "kpair.h"

#include "kghelpers.h"
#include "kgports.h"
#include "kgcontinuations.h" /* for guards */
#include "kgcontrol.h" /* for evaling in sequence */
#include "kgkd_vars.h" /* for dynamic input/output port */

/* 15.1.1 port? */
/* uses typep */

/* 15.1.2 input-port?, output-port? */
/* use ftypep */

/* 15.1.3 with-input-from-file, with-ouput-to-file */
/* helper for with-i/o-from/to-file & call-with-i/o-file */
void do_close_file_ret(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: port
    */

    TValue port = xparams[0];
    kclose_port(K, port);
    /* obj is the ret_val */
    kapply_cc(K, obj);
}

/* XXX: The report is incomplete here... for now use an empty environment, 
   the dynamic environment can be captured in the construction of the combiner 
   ASK John
*/
void with_file(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
    char *name = ksymbol_buf(xparams[0]);
    bool writep = bvalue(xparams[1]);
    TValue key = xparams[2];

    bind_2tp(K, name, ptree, "string", ttisstring, filename,
	     "combiner", ttiscombiner, comb);

    TValue new_port = kmake_port(K, filename, writep);
    krooted_tvs_push(K, new_port);
    /* make the continuation to close the file before returning */
    TValue new_cont = kmake_continuation(K, kget_cc(K), 
					 do_close_file_ret, 1, new_port);
    kset_cc(K, new_cont); /* cont implicitly rooted */
    krooted_tvs_pop(K); /* new_port is in cont */

    TValue op = kmake_operative(K, do_bind, 1, key);
    krooted_tvs_push(K, op);

    TValue args = klist(K, 2, new_port, comb);

    krooted_tvs_pop(K);

    /* even if we call with denv, do_bind calls comb in an empty env */
    /* XXX: what to pass for source info?? */
    ktail_call(K, op, args, denv);
}

/* 15.1.4 get-current-input-port, get-current-output-port */
void get_current_port(klisp_State *K, TValue *xparams, TValue ptree,
		      TValue denv)
{
    /*
    ** xparams[0]: symbol name
    ** xparams[1]: dynamic key
    */
    UNUSED(denv);

    char *name = ksymbol_buf(xparams[0]);
    TValue key = xparams[1];

    check_0p(K, name, ptree);

    /* can access directly, no need to call do_access */
    kapply_cc(K, kcdr(key));
}


/* 15.1.5 open-input-file, open-output-file */
void open_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    char *name = ksymbol_buf(xparams[0]);
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_1tp(K, name, ptree, "string", ttisstring, filename);

    TValue new_port = kmake_port(K, filename, writep);
    kapply_cc(K, new_port);
}

/* 15.1.6 close-input-file, close-output-file */
void close_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    char *name = ksymbol_buf(xparams[0]);
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_1tp(K, name, ptree, "port", ttisport, port);

    bool dir_ok = writep? kport_is_output(port) : kport_is_input(port);

    if (dir_ok) {
	kclose_port(K, port);
	kapply_cc(K, KINERT);
    } else {
	klispE_throw_extra(K, name, ": wrong input/output direction");
	return;
    }
}

/* 15.1.7 read */
void read(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;
    if (!get_opt_tpar(K, "read", K_TPORT, &port)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    } else if (!kport_is_input(port)) {
	klispE_throw(K, "read: the port should be an input port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw(K, "read: the port is already closed");
	return;
    }

    /* this may throw an error, that's ok */
    TValue obj = kread_from_port(K, port, true); /* read mutable pairs */ 
    kapply_cc(K, obj);
}

/* 15.1.8 write */
void write(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_al1tp(K, "write", ptree, "any", anytype, obj,
	       port);

    if (!get_opt_tpar(K, "write", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw(K, "write: the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw(K, "write: the port is already closed");
	return;
    }

    /* false: quote strings, escape chars */
    kwrite_display_to_port(K, port, obj, false); 
    kapply_cc(K, KINERT);
}

/* 15.1.? eof-object? */
/* uses typep */

/* 15.1.? newline */
void newline(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;
    if (!get_opt_tpar(K, "newline", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw(K, "write: the port should be an output port");
	return;
    }
    if (kport_is_closed(port)) {
	klispE_throw(K, "write: the port is already closed");
	return;
    }
    
    kwrite_newline_to_port(K, port);
    kapply_cc(K, KINERT);
}

/* 15.1.? write-char */
void write_char(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_al1tp(K, "write-char", ptree, "char", ttischar, ch,
	       port);

    if (!get_opt_tpar(K, "write-char", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw(K, "write-char: the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw(K, "write-char: the port is already closed");
	return;
    }
    
    kwrite_char_to_port(K, port, ch);
    kapply_cc(K, KINERT);
}

/* Helper for read-char and peek-char */
void read_peek_char(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
    /* 
    ** xparams[0]: symbol name
    ** xparams[1]: ret-char-after-readp
    */
    UNUSED(denv);
    
    char *name = ksymbol_buf(xparams[0]);
    bool ret_charp = bvalue(xparams[1]);

    TValue port = ptree;
    if (!get_opt_tpar(K, name, K_TPORT, &port)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    } else if (!kport_is_input(port)) {
	klispE_throw_extra(K, name, ": the port should be an input port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_extra(K, name, ": the port is already closed");
	return;
    }

    TValue obj = kread_peek_char_from_port(K, port, ret_charp);
    kapply_cc(K, obj);
}


/* 15.1.? read-char */
/* uses read_peek_char */

/* 15.1.? peek-char */
/* uses read_peek_char */

/* 15.1.? char-ready? */
/* XXX: this always return #t, proper behaviour requires platform 
   specific code (probably select for posix & a thread for windows
   (at least for files & consoles, I think pipes and sockets may
   have something) */
void char_readyp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;
    if (!get_opt_tpar(K, "char-ready?", K_TPORT, &port)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    } else if (!kport_is_input(port)) {
	klispE_throw(K, "char-ready?: the port should be an input port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw(K, "char-ready?: the port is already closed");
	return;
    }

    /* TODO: check if there are pending chars */
    kapply_cc(K, KTRUE);
}


/* 15.2.1 call-with-input-file, call-with-output-file */
/* XXX: The report is incomplete here... for now use an empty environment, 
   the dynamic environment can be captured in the construction of the combiner 
   ASK John
*/
void call_with_file(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
    char *name = ksymbol_buf(xparams[0]);
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_2tp(K, name, ptree, "string", ttisstring, filename,
	     "combiner", ttiscombiner, comb);

    TValue new_port = kmake_port(K, filename, writep);
    krooted_tvs_push(K, new_port);
    /* make the continuation to close the file before returning */
    TValue new_cont = kmake_continuation(K, kget_cc(K), 
					 do_close_file_ret, 1, new_port);
    kset_cc(K, new_cont); /* implicit rooting  */
    krooted_tvs_pop(K); /* new_port is in new_cont */
    TValue empty_env = kmake_empty_environment(K);
    krooted_tvs_push(K, empty_env);
    TValue expr = klist(K, 2, comb, new_port);

    krooted_tvs_pop(K);
    ktail_eval(K, expr, empty_env);
}

/* helpers for load */

/* read all expressions in a file, as immutable pairs */
/* GC: assume port is rooted */
TValue read_all_expr(klisp_State *K, TValue port)
{
    /* GC: root dummy and obj */
    TValue tail = kget_dummy1(K);
    TValue obj = KINERT;
    krooted_vars_push(K, &obj);

    while(true) {
	obj = kread_from_port(K, port, false); /* read immutable pairs */
	if (ttiseof(obj)) {
	    krooted_vars_pop(K);
	    return kcutoff_dummy1(K);
	} else {
	    TValue new_pair = kimm_cons(K, obj, KNIL);
	    kset_cdr_unsafe(K, tail, new_pair);
	    tail = new_pair;
	}
    }
}

/* interceptor for errors during reading, also for the continuat */
void do_int_close_file(klisp_State *K, TValue *xparams, TValue ptree, 
		   TValue denv)
{
    /*
    ** xparams[0]: port
    */
    UNUSED(denv);

    TValue port = xparams[0];
    /* ptree is (object divert) */
    TValue error_obj = kcar(ptree);
    kclose_port(K, port);
    /* pass the error along after closing the port */
    kapply_cc(K, error_obj);
}


/*
** guarded continuation making for read seq
*/

/* GC: assumes parent & port are rooted */
TValue make_guarded_read_cont(klisp_State *K, TValue parent, TValue port)
{
    /* create the guard to close file after read errors */
    TValue exit_int = kmake_operative(K, do_int_close_file, 
				      1, port);
    krooted_tvs_push(K, exit_int);
    TValue exit_guard = kcons(K, K->error_cont, exit_int);
    krooted_tvs_pop(K); /* alread in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* alread in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_guards = KNIL;

    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, parent, 
					   do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
					   do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);
    return inner_cont;
}

/* 15.2.2 load */
/* TEMP: this isn't yet defined in the report, but this seems pretty
   a sane way to do it: open the file whose name is passed
   as only parameter. read all the expressions in file as by read and
   accumulate them in a list. close the file. eval ($sequence . list) in
   the dynamic environment of the call to load. return #inert. If there is
   any error during reading, close the file and return that error.
   This is consistent with the report description of the load-module
   applicative.
   ASK John: maybe we should return the result of the last expression. 
*/
void load(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    bind_1tp(K, "load", ptree, "string", ttisstring, filename);

    /* the reads must be guarded to close the file if there is some error 
     this continuation also will return inert after the evaluation of the
     last expression is done */
    TValue port = kmake_port(K, filename, false);
    krooted_tvs_push(K, port);

    TValue inert_cont = make_return_value_cont(K, kget_cc(K), KINERT);
    krooted_tvs_push(K, inert_cont);

    TValue guarded_cont = make_guarded_read_cont(K, kget_cc(K), port);
    /* this will be used later, but contruct it now to use the 
       current continuation as parent 
    GC: root this obj */
    kset_cc(K, guarded_cont); /* implicit rooting */
    TValue ls = read_all_expr(K, port); /* any error will close the port */

    /* now the sequence of expresions should be evaluated in denv
       and #inert returned after all are done */
    kset_cc(K, inert_cont); /* implicit rooting */
    krooted_tvs_pop(K); /* already rooted */


    if (ttisnil(ls)) {
	krooted_tvs_pop(K); /* port */
	kapply_cc(K, KINERT);
    } else {
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    krooted_tvs_push(K, ls);
	    TValue new_cont = kmake_continuation(K, kget_cc(K),
						 do_seq, 2, tail, denv);
	    kset_cc(K, new_cont);
	    krooted_tvs_pop(K); /* ls */
	} 
	krooted_tvs_pop(K); /* port */
	ktail_eval(K, kcar(ls), denv);
    }
}

/* 15.2.3 get-module */
void get_module(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_al1tp(K, "get-module", ptree, "string", ttisstring, filename, 
	maybe_env);

    TValue port = kmake_port(K, filename, false);
    krooted_tvs_push(K, port);

    TValue env = kmake_environment(K, K->ground_env);
    krooted_tvs_push(K, env);

    if (get_opt_tpar(K, "", K_TENVIRONMENT, &maybe_env)) {
	kadd_binding(K, env, K->module_params_sym, maybe_env);
    }

    TValue ret_env_cont = make_return_value_cont(K, kget_cc(K), env);
    krooted_tvs_pop(K); /* env alread in cont */
    krooted_tvs_push(K, ret_env_cont);

    /* the reads must be guarded to close the file if there is some error 
     this continuation also will return inert after the evaluation of the
     last expression is done */
    TValue guarded_cont = make_guarded_read_cont(K, kget_cc(K), port);
    kset_cc(K, guarded_cont); /* implicit roooting */

    TValue ls = read_all_expr(K, port); /* any error will close the port */

    /* now the sequence of expresions should be evaluated in the created env
       and the environment returned after all are done */
    kset_cc(K, ret_env_cont); /* implicit rooting */
    krooted_tvs_pop(K); /* implicitly rooted */

    if (ttisnil(ls)) {
	krooted_tvs_pop(K); /* port */
	kapply_cc(K, KINERT);
    } else {
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    krooted_tvs_push(K, ls);
	    TValue new_cont = kmake_continuation(K, kget_cc(K),
					     do_seq, 2, tail, env);
	    kset_cc(K, new_cont);
	    krooted_tvs_pop(K);
	} 
	krooted_tvs_pop(K); /* port */
	ktail_eval(K, kcar(ls), env);
    }
}

/* 15.2.? display */
void display(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_al1tp(K, "display", ptree, "any", anytype, obj,
	       port);

    if (!get_opt_tpar(K, "display", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw(K, "display: the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw(K, "display: the port is already closed");
	return;
    }
    
    /* true: don't quote strings, don't escape chars */
    kwrite_display_to_port(K, port, obj, true); 
    kapply_cc(K, KINERT);
}
