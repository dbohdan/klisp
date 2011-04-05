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

    /* gc: root intermediate values */
    TValue new_port = kmake_port(K, filename, writep, KNIL, KNIL);
    /* make the continuation to close the file before returning */
    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL, 
					 do_close_file_ret, 1, new_port);
    kset_cc(K, new_cont);

    TValue op = kmake_operative(K, KNIL, KNIL, do_bind, 1, key);
    TValue args = kcons(K, new_port, kcons(K, comb, KNIL));
    /* even if we call with denv, do_bind calls comb in an empty env */
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

    TValue new_port = kmake_port(K, filename, writep, KNIL, KNIL);
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

    /* TEMP: for now set this by hand */
    K->curr_in = kport_file(port);
    ktok_reset_source_info(K); /* this should be saved in the port
				  and restored before the call to 
				  read and saved after it */
    K->read_cons_flag = true; /* read mutable pairs */
    TValue obj = kread(K); /* this may throw an error, that's ok */
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
    
    /* TEMP: for now set this by hand */
    K->curr_out = kport_file(port);

    kwrite(K, obj);
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
    
    /* TEMP: for now set this by hand */
    K->curr_out = kport_file(port);

    knewline(K);
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
    
    /* REFACTOR: move this to kwrite, update source info? */
    FILE *f = K->curr_out = kport_file(port);
    if (fputc(chvalue(ch), f) == EOF) {
	/* clear error marker to allow retries later */
	clearerr(f);
	klispE_throw(K, "write-char: writing error");
    } else {
	kapply_cc(K, KINERT);
    }
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

    /* TODO update source info on the port */
    FILE *f = K->curr_in = kport_file(port);
    int ch = fgetc(f);
    TValue obj;
    if (ch == EOF) {
	if (ferror(f) != 0) {
	    /* clear error marker to allow retries later */
	    clearerr(f);
	    klispE_throw_extra(K, name, ": reading error");
	    return;
	} else { /* if (feof(f) != 0) */
	    /* let the eof marker set */
	    obj = KEOF;
	}
    } else {
	obj = ch2tv((char) ch);
	/* check to see if this was a peek-char call */
	if (ret_charp) {
	    if (ungetc(ch, f) == EOF)  {
		/* shouldn't happen, but better be safe than sorry */
		/* clear error marker to allow retries later */
		clearerr(f);
		klispE_throw_extra(K, name, ": error ungetting char");
		return;
	    }
	}
    }
    kapply_cc(K, obj);
}


/* 15.1.? read-char */
/* TODO */

/* 15.1.? peek-char */
/* TODO */

/* 15.1.? char-ready? */
/* TODO */
/* XXX: this always return #t, proper behaviour requires platform 
   specific code (probably select for posix, a thread for windows
   (at least for files & consoles), I think pipes and sockets may
   have something */

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

    /* gc: root intermediate values */
    TValue empty_env = kmake_empty_environment(K);
    TValue new_port = kmake_port(K, filename, writep, KNIL, KNIL);
    TValue expr = kcons(K, comb, kcons(K, new_port, KNIL));

    /* make the continuation to close the file before returning */
    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL, 
					 do_close_file_ret, 1, new_port);
    kset_cc(K, new_cont);
    ktail_eval(K, expr, empty_env);
}

/* helpers for load */

/* read all expressions in a file, as immutable pairs */
TValue read_all_expr(klisp_State *K, TValue port)
{
    /* TEMP: for now set this by hand */
    K->curr_in = kport_file(port);
    ktok_reset_source_info(K);
    K->read_cons_flag = false; /* read immutable pairs */

    /* GC: root dummy and obj */
    TValue dummy = kimm_cons(K, KNIL, KNIL);
    TValue tail = dummy;
    TValue obj = KINERT;

    while(true) {
	obj = kread(K);
	if (ttiseof(obj)) {
	    return kcdr(dummy);
	} else {
	    TValue new_pair = kimm_cons(K, obj, KNIL);
	    kset_cdr(tail, new_pair);
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
TValue make_guarded_read_cont(klisp_State *K, TValue parent, TValue port)
{
    /* create the guard to close file after read errors */
    TValue exit_int = kmake_operative(K, KNIL, KNIL, do_int_close_file, 
				      1, port);
    TValue exit_guard = kcons(K, K->error_cont, exit_int);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    TValue entry_guards = KNIL;
    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    TValue outer_cont = kmake_continuation(K, parent, KNIL, KNIL, 
					   do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, KNIL, KNIL, 
					   do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);
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
    TValue port = kmake_port(K, filename, false, KNIL, KNIL);
    TValue guarded_cont = make_guarded_read_cont(K, kget_cc(K), port);
    /* this will be used later, but contruct it now to use the 
       current continuation as parent 
    GC: root this obj */
    TValue inert_cont = make_return_value_cont(K, kget_cc(K), KINERT);

    kset_cc(K, guarded_cont);
    TValue ls = read_all_expr(K, port); /* any error will close the port */

    /* now the sequence of expresions should be evaluated in denv
       and #inert returned after all are done */
    kset_cc(K, inert_cont);

    if (ttisnil(ls)) {
	kapply_cc(K, KINERT);
    } else {
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, denv);
	    kset_cc(K, new_cont);
	} 
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

    TValue env = kmake_environment(K, K->ground_env);

    if (get_opt_tpar(K, "", K_TENVIRONMENT, &maybe_env)) {
	kadd_binding(K, env, K->module_params_sym, maybe_env);
    }

    /* the reads must be guarded to close the file if there is some error 
     this continuation also will return inert after the evaluation of the
     last expression is done */
    TValue port = kmake_port(K, filename, false, KNIL, KNIL);
    TValue guarded_cont = make_guarded_read_cont(K, kget_cc(K), port);
    /* this will be used later, but contruct it now to use the 
       current continuation as parent 
    GC: root this obj */
    TValue ret_env_cont = make_return_value_cont(K, kget_cc(K), env);

    kset_cc(K, guarded_cont);
    TValue ls = read_all_expr(K, port); /* any error will close the port */

    /* now the sequence of expresions should be evaluated in the created env
       and the environment returned after all are done */
    kset_cc(K, ret_env_cont);

    if (ttisnil(ls)) {
	kapply_cc(K, KINERT);
    } else {
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, env);
	    kset_cc(K, new_cont);
	} 
	ktail_eval(K, kcar(ls), env);
    }
}

/* 15.2.? display */
/* TODO */
