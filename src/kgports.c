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
#include "kstring.h"
#include "kbytevector.h"
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

/* 15.1.? binary-port?, textual-port? */
/* use ftypep */

/* 15.1.? file-port?, string-port?, bytevector-port? */
/* use ftypep */

/* 15.1.? port-open? */
/* uses ftyped_predp */

/* uses ftyped_predp */

/* 15.1.3 with-input-from-file, with-ouput-to-file */
/* helper for with-i/o-from/to-file & call-with-i/o-file */
void do_close_file_ret(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
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

    TValue new_port = kmake_fport(K, filename, writep, false);
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
/* 15.1.? open-binary-input-file, open-binary-output-file */
void open_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: write?
    ** xparams[1]: binary?
    */
    bool writep = bvalue(xparams[0]);
    bool binaryp = bvalue(xparams[1]);

    bind_1tp(K, ptree, "string", ttisstring, filename);

    TValue new_port = kmake_fport(K, filename, writep, binaryp);
    kapply_cc(K, new_port);
}

/* 15.1.? open-input-string, open-output-string */
/* 15.1.? open-input-bytevector, open-output-bytevector */
void open_mport(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: write?
    ** xparams[1]: binary?
    */
    bool writep = bvalue(xparams[0]);
    bool binaryp = bvalue(xparams[1]);
    UNUSED(denv);

    TValue buffer;
    
    /* This is kinda ugly but... */
    if (writep) {
	check_0p(K, ptree);
	buffer = KINERT;
    } else if (binaryp) {
	bind_1tp(K, ptree, "bytevector", ttisbytevector, bb);
	buffer = bb;
    } else {
	bind_1tp(K, ptree, "string", ttisstring, str);
	buffer = str;
    }

    TValue new_port = kmake_mport(K, buffer, writep, binaryp);
    kapply_cc(K, new_port);
}

/* 15.1.? open-output-string, open-output-bytevector */

/* 15.1.6 close-input-file, close-output-file */
void close_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: write?
    */
    bool writep = bvalue(xparams[0]);
    UNUSED(denv);

    bind_1tp(K, ptree, "file port", ttisfport, port);

    bool dir_ok = writep? kport_is_output(port) : kport_is_input(port);

    if (dir_ok) {
	kclose_port(K, port);
	kapply_cc(K, KINERT);
    } else {
	klispE_throw_simple(K, "wrong input/output direction");
	return;
    }
}

/* 15.1.? close-input-port, close-output-port, close-port */
void close_port(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: read?
    ** xparams[1]: write?
    */
    bool readp = bvalue(xparams[0]);
    bool writep = bvalue(xparams[1]);
    UNUSED(denv);

    bind_1tp(K, ptree, "port", ttisport, port);

    bool dir_ok = !((writep && !kport_is_output(port)) ||
		    (readp && !kport_is_input(port)));

    if (dir_ok) {
	kclose_port(K, port);
	kapply_cc(K, KINERT);
    } else {
	klispE_throw_simple(K, "wrong input/output direction");
	return;
    }
}

/* 15.1.? get-output-string, get-output-bytevector */
void get_output_buffer(klisp_State *K, TValue *xparams, TValue ptree, 
		       TValue denv)
{
    /*
    ** xparams[0]: binary?
    */
    bool binaryp = bvalue(xparams[0]);
    UNUSED(denv);
    bind_1tp(K, ptree, "port", ttismport, port);

    if (binaryp && !kport_is_binary(port)) {
	klispE_throw_simple(K, "the port should be a bytevector port");
	return;
    } else if (!binaryp && !kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a string port");
	return;
    } else if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    }
    
    TValue ret = binaryp? 
	kbytevector_new_bs(K, 
			   kbytevector_buf(kmport_buf(port)), 
			   kmport_off(port)) :
	kstring_new_bs(K, kstring_buf(kmport_buf(port)), kmport_off(port));
    kapply_cc(K, ret);
}

/* 15.1.7 read */
void gread(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;
    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    } 

    if (!kport_is_input(port)) {
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }

    /* this may throw an error, that's ok */
    TValue obj = kread_from_port(K, port, true); /* read mutable pairs */ 
    kapply_cc(K, obj);
}

/* 15.1.8 write */
void gwrite(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_al1tp(K, ptree, "any", anytype, obj,
	       port);

    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } 

    if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
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
    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    }

    if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
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

    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } 

    if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
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
    ** xparams[0]: ret-char-after-readp
    */
    UNUSED(denv);
    
    bool ret_charp = bvalue(xparams[0]);

    TValue port = ptree;
    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    } 
    
    if (!kport_is_input(port)) {
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
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
    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    } 

    if (!kport_is_input(port)) {
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }

    /* TODO: check if there are pending chars */
    kapply_cc(K, KTRUE);
}

/* 15.1.? write-u8 */
void write_u8(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_al1tp(K, ptree, "u8", ttisu8, u8, port);

    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    } 

    if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } else if (!kport_is_binary(port)) {
	klispE_throw_simple(K, "the port should be a binary port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }
    
    kwrite_u8_to_port(K, port, u8);
    kapply_cc(K, KINERT);
}

/* Helper for read-u8 and peek-u8 */
void read_peek_u8(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
    /* 
    ** xparams[0]: ret-u8-after-readp
    */
    UNUSED(denv);
    
    bool ret_u8p = bvalue(xparams[0]);

    TValue port = ptree;
    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    }

    if (!kport_is_input(port)) {
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } else if (!kport_is_binary(port)) {
	klispE_throw_simple(K, "the port should be a binary port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }

    TValue obj = kread_peek_u8_from_port(K, port, ret_u8p);
    kapply_cc(K, obj);
}


/* 15.1.? read-u8 */
/* uses read_peek_u8 */

/* 15.1.? peek-u8 */
/* uses read_peek_u8 */

/* 15.1.? u8-ready? */
/* XXX: this always return #t, proper behaviour requires platform 
   specific code (probably select for posix & a thread for windows
   (at least for files & consoles, I think pipes and sockets may
   have something) */
void u8_readyp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;
    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_in_port_key); /* access directly */
    }
    
    if (!kport_is_input(port)) {
	klispE_throw_simple(K, "the port should be an input port");
	return;
    } else if (!kport_is_binary(port)) {
	klispE_throw_simple(K, "the port should be a binary port");
	return;
    }  else if (kport_is_closed(port)) {
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

    TValue new_port = kmake_fport(K, filename, writep, false);
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

/* interceptor for errors during reading */
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
    TValue port = kmake_fport(K, filename, false, false);
    krooted_tvs_push(K, port);

    TValue inert_cont = make_return_value_cont(K, kget_cc(K), KINERT);
    krooted_tvs_push(K, inert_cont);

    TValue guarded_cont = make_guarded_read_cont(K, kget_cc(K), port);
    /* this will be used later, but contruct it now to use the 
       current continuation as parent 
    GC: root this obj */
    kset_cc(K, guarded_cont); /* implicit rooting */
    /* any error will close the port */
    TValue ls = kread_list_from_port(K, port, false);  /* immutable pairs */

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

    TValue port = kmake_fport(K, filename, false, false);
    krooted_tvs_push(K, port);

    /* std environments have hashtable for bindings */
    TValue env = kmake_table_environment(K, K->ground_env);
//    TValue env = kmake_environment(K, K->ground_env);
    krooted_tvs_push(K, env);

    if (get_opt_tpar(K, maybe_env, "environment", ttisenvironment)) {
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

    
    /* any error will close the port */
    TValue ls = kread_list_from_port(K, port, false); /* use immutable pairs */

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

    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    }

    if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } else if (!kport_is_textual(port)) {
	klispE_throw_simple(K, "the port should be a textual port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }
    
    /* true: don't quote strings, don't escape chars */
    kwrite_display_to_port(K, port, obj, true); 
    kapply_cc(K, KINERT);
}

/* 15.1.? flush-output-port */
void flush(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    TValue port = ptree;

    if (!get_opt_tpar(K, port, "port", ttisport)) {
	port = kcdr(K->kd_out_port_key); /* access directly */
    }

    if (!kport_is_output(port)) {
	klispE_throw_simple(K, "the port should be an output port");
	return;
    } 

    if (kport_is_closed(port)) {
	klispE_throw_simple(K, "the port is already closed");
	return;
    }

    kwrite_flush_port(K, port);
    kapply_cc(K, KINERT);
}

/* 15.1.? file-exists? */
void file_existsp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, filename);

    /* TEMP: this should probably be done in a operating system specific
       manner, but this will do for now */
    TValue res = KFALSE;
    FILE *file = fopen(kstring_buf(filename), "r");
    if (file) {
	res = KTRUE;
	UNUSED(fclose(file));
    }
    kapply_cc(K, res);
}

/* 15.1.? delete-file */
void delete_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, filename);

    /* TEMP: this should probably be done in a operating system specific
       manner, but this will do for now */
    /* XXX: this could fail if there's a dead (in the gc sense) port still 
       open, should probably retry once after doing a complete GC */
    if (remove(kstring_buf(filename))) {
        klispE_throw_errno_with_irritants(K, "remove", 1, filename);
        return;
    } else {
	kapply_cc(K, KINERT);
	return;
    }
}

/* 15.1.? rename-file */
void rename_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_2tp(K, ptree, "string", ttisstring, old_filename, 
	     "string", ttisstring, new_filename);

    /* TEMP: this should probably be done in a operating system specific
       manner, but this will do for now */
    /* XXX: this could fail if there's a dead (in the gc sense) port still 
       open, should probably retry once after doing a complete GC */
    if (rename(kstring_buf(old_filename), kstring_buf(new_filename))) {
        klispE_throw_errno_with_irritants(K, "rename", 2, old_filename, new_filename);
        return;
    } else {
	kapply_cc(K, KINERT);
	return;
    }
}

/* init ground */
void kinit_ports_ground_env(klisp_State *K)
{
    /*
    ** Some of these are from r7rs scheme
    */

    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 15.1.1 port? */
    add_applicative(K, ground_env, "port?", ftypep, 2, symbol, 
		    p2tv(kportp));
    /* 15.1.2 input-port?, output-port? */
    add_applicative(K, ground_env, "input-port?", ftypep, 2, symbol, 
		    p2tv(kinput_portp));
    add_applicative(K, ground_env, "output-port?", ftypep, 2, symbol, 
		    p2tv(koutput_portp));
    /* 15.1.? binary-port?, textual-port? */
    add_applicative(K, ground_env, "binary-port?", ftypep, 2, symbol, 
		    p2tv(kbinary_portp));
    add_applicative(K, ground_env, "textual-port?", ftypep, 2, symbol, 
		    p2tv(ktextual_portp));
    /* 15.1.2 file-port?, string-port?, bytevector-port? */
    add_applicative(K, ground_env, "file-port?", ftypep, 2, symbol, 
		    p2tv(kfile_portp));
    add_applicative(K, ground_env, "string-port?", ftypep, 2, symbol, 
		    p2tv(kstring_portp));
    add_applicative(K, ground_env, "bytevector-port?", ftypep, 2, symbol, 
		    p2tv(kbytevector_portp));
    /* 15.1.? port-open? */
    add_applicative(K, ground_env, "port-open?", ftyped_predp, 3, symbol, 
		    p2tv(kportp), p2tv(kport_openp));

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
    add_applicative(K, ground_env, "open-input-file", open_file, 2, 
		    b2tv(false), b2tv(false));
    add_applicative(K, ground_env, "open-output-file", open_file, 2, 
		    b2tv(true), b2tv(false));
    /* 15.1.? open-binary-input-file, open-binary-output-file */
    add_applicative(K, ground_env, "open-binary-input-file", open_file, 2, 
		    b2tv(false), b2tv(true));
    add_applicative(K, ground_env, "open-binary-output-file", open_file, 2, 
		    b2tv(true), b2tv(true));
    /* 15.1.? open-input-string, open-output-string */
    /* 15.1.? open-input-bytevector, open-output-bytevector */
    add_applicative(K, ground_env, "open-input-string", open_mport, 2, 
		    b2tv(false), b2tv(false));
    add_applicative(K, ground_env, "open-output-string", open_mport, 2, 
		    b2tv(true), b2tv(false));
    add_applicative(K, ground_env, "open-input-bytevector", open_mport, 2, 
		    b2tv(false), b2tv(true));
    add_applicative(K, ground_env, "open-output-bytevector", open_mport, 2, 
		    b2tv(true), b2tv(true));

    /* 15.1.6 close-input-file, close-output-file */
    /* ASK John: should this be called close-input-port & close-ouput-port 
       like in r5rs? that doesn't seem consistent with open thou */
    add_applicative(K, ground_env, "close-input-file", close_file, 1,
		    b2tv(false));
    add_applicative(K, ground_env, "close-output-file", close_file, 1,
		    b2tv(true));
    /* 15.1.? Use the r7rs names, in preparation for other kind of ports */
    add_applicative(K, ground_env, "close-input-port", close_port, 2, 
		    b2tv(true), b2tv(false));
    add_applicative(K, ground_env, "close-output-port", close_port, 2, 
		    b2tv(false), b2tv(true));
    add_applicative(K, ground_env, "close-port", close_port, 2, 
		    b2tv(false), b2tv(false));

    /* 15.1.? get-output-string, get-output-bytevector */
    add_applicative(K, ground_env, "get-output-string", get_output_buffer, 1, 
		    b2tv(false));
    add_applicative(K, ground_env, "get-output-bytevector", get_output_buffer, 
		    1, b2tv(true));

    /* 15.1.7 read */
    add_applicative(K, ground_env, "read", gread, 0);
    /* 15.1.8 write */
    add_applicative(K, ground_env, "write", gwrite, 0);

    /* 15.1.? eof-object? */
    add_applicative(K, ground_env, "eof-object?", typep, 2, symbol, 
		    i2tv(K_TEOF));
    /* 15.1.? newline */
    add_applicative(K, ground_env, "newline", newline, 0);
    /* 15.1.? write-char */
    add_applicative(K, ground_env, "write-char", write_char, 0);
    /* 15.1.? read-char */
    add_applicative(K, ground_env, "read-char", read_peek_char, 1, 
		    b2tv(false));
    /* 15.1.? peek-char */
    add_applicative(K, ground_env, "peek-char", read_peek_char, 1,
		    b2tv(true));
    /* 15.1.? char-ready? */
    /* XXX: this always return #t, proper behaviour requires platform 
       specific code (probably select for posix, a thread for windows
       (at least for files & consoles), I think pipes and sockets may
       have something */
    add_applicative(K, ground_env, "char-ready?", char_readyp, 0);
    /* 15.1.? write-u8 */
    add_applicative(K, ground_env, "write-u8", write_u8, 0);
    /* 15.1.? read-u8 */
    add_applicative(K, ground_env, "read-u8", read_peek_u8, 1, 
		    b2tv(false));
    /* 15.1.? peek-u8 */
    add_applicative(K, ground_env, "peek-u8", read_peek_u8, 1, 
		    b2tv(true));
    /* 15.1.? u8-ready? */
    /* XXX: this always return #t, proper behaviour requires platform 
       specific code (probably select for posix, a thread for windows
       (at least for files & consoles), I think pipes and sockets may
       have something */
    add_applicative(K, ground_env, "u8-ready?", u8_readyp, 0);
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

    /* 15.1.? flush-output-port */
    add_applicative(K, ground_env, "flush-output-port", flush, 0);

    /* REFACTOR move to system module */

    /* 15.1.? file-exists? */
    add_applicative(K, ground_env, "file-exists?", file_existsp, 0);

    /* 15.1.? delete-file */
    add_applicative(K, ground_env, "delete-file", delete_file, 0);

    /* this isn't in r7rs but it's in ansi c and quite easy to implement */

    /* 15.1.? rename-file */
    add_applicative(K, ground_env, "rename-file", rename_file, 0);

    /*
     * That's all there is in the report combined with r5rs and r7rs scheme.
     * TODO
     * It would be good to be able to select between append, truncate and
     * error if a file exists, but that would need to be an option in all three 
     * methods of opening. Also some directory checking, traversing, etc,
     * would be nice
     */
}
