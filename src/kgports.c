/*
** kgports.c
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

#include "kscript.h"

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
    bool writep = bvalue(xparams[1]);
    TValue key = xparams[2];

    bind_2tp(K, ptree, "string", ttisstring, filename,
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

    TValue key = xparams[1];

    check_0p(K, ptree);

    /* can access directly, no need to call do_access */
    kapply_cc(K, kcdr(key));
}


/* 15.1.5 open-input-file, open-output-file */
void open_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, filename);

    TValue new_port = kmake_port(K, filename, writep);
    kapply_cc(K, new_port);
}

/* 15.1.6 close-input-file, close-output-file */
void close_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_1tp(K, ptree, "port", ttisport, port);

    bool dir_ok = writep? kport_is_output(port) : kport_is_input(port);

    if (dir_ok) {
	kclose_port(K, port);
	kapply_cc(K, KINERT);
    } else {
	klispE_throw_simple(K, "wrong input/output direction");
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
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
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
    
    bind_al1tp(K, ptree, "any", anytype, obj,
	       port);

    if (!get_opt_tpar(K, "write", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
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
	klispE_throw_simple(K, "the port should be an output port");
	return;
    }
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
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
    
    bind_al1tp(K, ptree, "char", ttischar, ch,
	       port);

    if (!get_opt_tpar(K, "write-char", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
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
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
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
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
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
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_2tp(K, ptree, "string", ttisstring, filename,
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
    /* support unix script directive #! */
    int line_count = kscript_eat_directive(kport_file(port));
    kport_line(port) += line_count;

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
#if KTRACK_SI
	    /* put the source info */
	    /* XXX: should first read all comments and whitespace,
	     then save the source info, then read the object and
	     lastly put the saved source info on the new pair...
	     For now this will do, but it's not technically correct */
	    kset_source_info(K, new_pair, ktry_get_si(K, obj));
#endif
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
    bind_1tp(K, ptree, "string", ttisstring, filename);

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
#if KTRACK_SI
	    /* put the source info of the list including the element
	       that we are about to evaluate */
	    kset_source_info(K, new_cont, ktry_get_si(K, ls));
#endif
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
    bind_al1tp(K, ptree, "string", ttisstring, filename, 
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
#if KTRACK_SI
	    /* put the source info of the list including the element
	       that we are about to evaluate */
	    kset_source_info(K, new_cont, ktry_get_si(K, ls));
#endif
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
    
    bind_al1tp(K, ptree, "any", anytype, obj,
	       port);

    if (!get_opt_tpar(K, "display", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }
    
    /* true: don't quote strings, don't escape chars */
    kwrite_display_to_port(K, port, obj, true); 
    kapply_cc(K, KINERT);
}

/* 15.1.? flush-output-port */
void kflush(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;

    if (!get_opt_tpar(K, "flush-output-port", K_TPORT, &port)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } else if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } 
    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }

    FILE *file = kport_file(port);
    if (file) { /* only do for file ports */
	UNUSED(fflush(file)); /* TEMP for now don't signal errors on flush */
    }
    kapply_cc(K, KINERT);
}

/* init ground */
void kinit_ports_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 15.1.1 port? */
    add_applicative(K, ground_env, "port?", typep, 2, symbol, 
		    i2tv(K_TPORT));
    /* 15.1.2 input-port?, output-port? */
    add_applicative(K, ground_env, "input-port?", ftypep, 2, symbol, 
		    p2tv(kis_input_port));
    add_applicative(K, ground_env, "output-port?", ftypep, 2, symbol, 
		    p2tv(kis_output_port));
    /* 15.1.3 with-input-from-file, with-ouput-to-file */
    /* 15.1.? with-error-to-file */
    add_applicative(K, ground_env, "with-input-from-file", with_file, 
		    3, symbol, b2tv(false), K->kd_in_port_key);
    add_applicative(K, ground_env, "with-output-to-file", with_file, 
		    3, symbol, b2tv(true), K->kd_out_port_key);
    add_applicative(K, ground_env, "with-error-to-file", with_file, 
		    3, symbol, b2tv(true), K->kd_error_port_key);
    /* 15.1.4 get-current-input-port, get-current-output-port */
    /* 15.1.? get-current-error-port */
    add_applicative(K, ground_env, "get-current-input-port", get_current_port, 
		    2, symbol, K->kd_in_port_key);
    add_applicative(K, ground_env, "get-current-output-port", get_current_port, 
		    2, symbol, K->kd_out_port_key);
    add_applicative(K, ground_env, "get-current-error-port", get_current_port, 
		    2, symbol, K->kd_error_port_key);
    /* 15.1.5 open-input-file, open-output-file */
    add_applicative(K, ground_env, "open-input-file", open_file, 2, symbol, 
		    b2tv(false));
    add_applicative(K, ground_env, "open-output-file", open_file, 2, symbol, 
		    b2tv(true));
    /* 15.1.6 close-input-file, close-output-file */
    /* ASK John: should this be called close-input-port & close-ouput-port 
       like in r5rs? that doesn't seem consistent with open thou */
    add_applicative(K, ground_env, "close-input-file", close_file, 2, symbol, 
		    b2tv(false));
    add_applicative(K, ground_env, "close-output-file", close_file, 2, symbol, 
		    b2tv(true));
    /* 15.1.7 read */
    add_applicative(K, ground_env, "read", read, 0);
    /* 15.1.8 write */
    add_applicative(K, ground_env, "write", write, 0);

    /*
    ** These are from scheme (r5rs)
    */

    /* 15.1.? eof-object? */
    add_applicative(K, ground_env, "eof-object?", typep, 2, symbol, 
		    i2tv(K_TEOF));
    /* 15.1.? newline */
    add_applicative(K, ground_env, "newline", newline, 0);
    /* 15.1.? write-char */
    add_applicative(K, ground_env, "write-char", write_char, 0);
    /* 15.1.? read-char */
    add_applicative(K, ground_env, "read-char", read_peek_char, 2, symbol, 
		    b2tv(false));
    /* 15.1.? peek-char */
    add_applicative(K, ground_env, "peek-char", read_peek_char, 2, symbol, 
		    b2tv(true));
    /* 15.1.? char-ready? */
    /* XXX: this always return #t, proper behaviour requires platform 
       specific code (probably select for posix, a thread for windows
       (at least for files & consoles), I think pipes and sockets may
       have something */
    add_applicative(K, ground_env, "char-ready?", char_readyp, 0);
    /* 15.2.1 call-with-input-file, call-with-output-file */
    add_applicative(K, ground_env, "call-with-input-file", call_with_file, 
		    2, symbol, b2tv(false));
    add_applicative(K, ground_env, "call-with-output-file", call_with_file, 
		    2, symbol, b2tv(true));
    /* 15.2.2 load */
    add_applicative(K, ground_env, "load", load, 0);
    /* 15.2.3 get-module */
    add_applicative(K, ground_env, "get-module", get_module, 0);
    /* 15.2.? display */
    add_applicative(K, ground_env, "display", display, 0);

    /* That's all there is in the report combined with r5rs scheme, 
       but we will probably need: file-exists?, rename-file and remove-file.
       It would also be good to be able to select between append, truncate and
       error if a file exists, but that would need to be an option in all three 
       methods of opening. Also some directory checking, traversing etc */
    /* BUT SEE r7rs draft for some of the above */
    /* r7rs */

    /* 15.1.? flush-output-port */
    add_applicative(K, ground_env, "flush-output-port", kflush, 0);
}
