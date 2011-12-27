/*
** kgports.c
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "kstate.h"
#include "kobject.h"
#include "kport.h"
#include "kstring.h"
#include "ktable.h"
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

#include "kghelpers.h"
#include "kgports.h"

/* Continuations */
void do_close_file_ret(klisp_State *K);

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
void with_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void get_current_port(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void open_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);

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
void open_mport(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void close_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void close_port(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void get_output_buffer(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void gread(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void gwrite(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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

/* 15.1.? write-simple */
void gwrite_simple(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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

    kwrite_simple_to_port(K, port, obj); 
    kapply_cc(K, KINERT);
}

/* 15.1.? eof-object? */
/* uses typep */

/* 15.1.? newline */
void newline(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void write_char(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void read_peek_char(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void char_readyp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void write_u8(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void read_peek_u8(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void u8_readyp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void call_with_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void do_int_close_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
void load(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    bind_1tp(K, ptree, "string", ttisstring, filename);

    /* the reads must be guarded to close the file if there is some error 
       this continuation also will return inert after the evaluation of the
       last expression is done */
    TValue port = kmake_fport(K, filename, false, false);
    krooted_tvs_push(K, port);

    TValue inert_cont = kmake_continuation(K, kget_cc(K), do_return_value, 1, 
                                           KINERT);
    
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

/* Helpers for require */
static bool readable(const char *filename) {
    FILE *f = fopen(filename, "r");  /* try to open file */
    if (f == NULL) return false;  /* open failed */
    fclose(f);
    return true;
}

/* Path can't/shouldn't contain embedded zeros */
static const char *get_next_template(klisp_State *K, const char *path, 
                                     TValue *next) {
    const char *l;
    while (*path == *KLISP_PATHSEP) path++;  /* skip separators */
    if (*path == '\0') return NULL;  /* no more templates */
    l = strchr(path, *KLISP_PATHSEP);  /* find next separator */
    if (l == NULL) l = path + strlen(path);
    *next = kstring_new_bs(K, path, l-path); /* template */
    return l; /* pointer to the end of the template */
}

/* no strings should contains embedded zeroes */
static TValue str_sub(klisp_State *K, TValue s, TValue p, TValue r)
{
    const char *sp = kstring_buf(s);
    const char *pp = kstring_buf(p);
    const char *rp = kstring_buf(r);

    uint32_t size = kstring_size(s);
    uint32_t psize = kstring_size(p);
    uint32_t rsize = kstring_size(r);
    int32_t diff_size = rsize - psize;

    const char *wild;

    /* first calculate needed size */
    while ((wild = strstr(sp, pp)) != NULL) {
        size += diff_size;
        sp = wild + psize;
    }

    /* now construct result buffer and fill it */
    TValue res = kstring_new_s(K, size);
    char *resp = kstring_buf(res);
    sp = kstring_buf(s);
    while ((wild = strstr(sp, pp)) != NULL) {
        ptrdiff_t l = wild - sp;
        memcpy(resp, sp, l);
        resp += l;
        memcpy(resp, rp, rsize);
        resp += rsize;
        sp = wild + psize;
    }
    strcpy(resp, sp); /* the size was calculated beforehand */
    return res;
}

static TValue find_file (klisp_State *K, TValue name, TValue pname) {
    /* not used in klisp */
    /* name = luaL_gsub(L, name, ".", LUA_DIRSEP); */
    /* lua_getfield(L, LUA_ENVIRONINDEX, pname); */
    klisp_assert(ttisstring(name) && !kstring_emptyp(name));
    const char *path = kstring_buf(pname);
    TValue next = K->empty_string;
    krooted_vars_push(K, &next);
    TValue wild = kstring_new_b(K, KLISP_PATH_MARK);
    krooted_tvs_push(K, wild);

    while ((path = get_next_template(K, path, &next)) != NULL) {
        next = str_sub(K, next, wild, name);
        if (readable(kstring_buf(next))) {  /* does file exist and is readable? */
            krooted_tvs_pop(K);
            krooted_vars_pop(K);
            return next;  /* return that file name */
        }
    }
    
    krooted_tvs_pop(K);
    krooted_vars_pop(K);
    return K->empty_string;  /* return empty_string */
}

/* ?.? require */
/*
** require is like load except that:
**  - require first checks to see if the file was already required
**    and if so, doesnt' do anything
**  - require looks for the named file in a number of locations
**    configurable via env var KLISP_PATH
**  - When/if the file is found, evaluation happens in an initially
**   standard environment
*/
void require(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    UNUSED(xparams);
    bind_1tp(K, ptree, "string", ttisstring, name);

    if (kstring_emptyp(name)) {
        klispE_throw_simple(K, "Empty name");
        return;
    }
    /* search for the named file in the table of already
       required files. 
       N.B. this will be fooled if the same file is accessed
       through different names */
    TValue saved_name = kstring_immutablep(name)? name :
        kstring_new_bs_imm(K, kstring_buf(name), kstring_size(name));

    const TValue *node = klispH_getstr(tv2table(K->require_table), 
                                       tv2str(saved_name));
    if (!ttisfree(*node)) {
        /* was required already, nothing to be done */
        kapply_cc(K, KINERT);
    }

    krooted_tvs_push(K, saved_name);
    TValue filename = K->empty_string;
    krooted_vars_push(K, &filename);
    filename = find_file(K, name, K->require_path);
    
    if (kstring_emptyp(filename)) {
        klispE_throw_simple_with_irritants(K, "Not found", 1, name);
        return;
    }

    /* the file was found, save it in the table */
    /* MAYBE the name should be saved in the table only if no error
       occured... but that could lead to loops if the file is
       required recursively. A third option would be to record the 
       sate of the require in the table, so we could have: error, required,
       requiring, etc */
    *(klispH_setstr(K, tv2table(K->require_table), tv2str(saved_name))) = 
        KTRUE;
    krooted_tvs_pop(K); /* saved_name no longer necessary */

    /* the reads must be guarded to close the file if there is some error 
       this continuation also will return inert after the evaluation of the
       last expression is done */
    TValue port = kmake_fport(K, filename, false, false);
    krooted_tvs_push(K, port);
    krooted_vars_pop(K); /* filename already rooted */

    TValue inert_cont = kmake_continuation(K, kget_cc(K), do_return_value, 1, 
                                           KINERT);
    
    krooted_tvs_push(K, inert_cont);

    TValue guarded_cont = make_guarded_read_cont(K, kget_cc(K), port);
    /* this will be used later, but contruct it now to use the 
       current continuation as parent 
       GC: root this obj */
    kset_cc(K, guarded_cont); /* implicit rooting */
    /* any error will close the port */
    TValue ls = kread_list_from_port(K, port, false);  /* immutable pairs */

    /* now the sequence of expresions should be evaluated in a
       standard environment and #inert returned after all are done */
    kset_cc(K, inert_cont); /* implicit rooting */
    krooted_tvs_pop(K); /* already rooted */

    if (ttisnil(ls)) {
        krooted_tvs_pop(K); /* port */
        kapply_cc(K, KINERT);
    } else {
        TValue tail = kcdr(ls);
        /* std environments have hashtable for bindings */
        TValue env = kmake_table_environment(K, K->ground_env);
        if (ttispair(tail)) {
            krooted_tvs_push(K, ls);
            krooted_tvs_push(K, env);
            TValue new_cont = kmake_continuation(K, kget_cc(K),
                                                 do_seq, 2, tail, env);
            kset_cc(K, new_cont);
#if KTRACK_SI
            /* put the source info of the list including the element
               that we are about to evaluate */
            kset_source_info(K, new_cont, ktry_get_si(K, ls));
#endif
            krooted_tvs_pop(K); /* env */
            krooted_tvs_pop(K); /* ls */
        } 
        krooted_tvs_pop(K); /* port */
        ktail_eval(K, kcar(ls), env);
    }
}

/* ?.? registered-requirement? */
void registered_requirementP(klisp_State *K)
{
    bind_1tp(K, K->next_value, "string", ttisstring, name);
    if (kstring_emptyp(name)) {
        klispE_throw_simple(K, "Empty name");
        return;
    }
    /* search for the named file in the table of already
       required files. 
       N.B. this will be fooled if the same file is accessed
       through different names */
    TValue saved_name = kstring_immutablep(name)? name :
        kstring_new_bs_imm(K, kstring_buf(name), kstring_size(name));

    const TValue *node = klispH_getstr(tv2table(K->require_table), 
                                       tv2str(saved_name));
    kapply_cc(K, ttisfree(*node)? KFALSE : KTRUE);
}

void register_requirementB(klisp_State *K)
{
    bind_1tp(K, K->next_value, "string", ttisstring, name);
    if (kstring_emptyp(name)) {
        klispE_throw_simple(K, "Empty name");
        return;
    }
    TValue saved_name = kstring_immutablep(name)? name :
        kstring_new_bs_imm(K, kstring_buf(name), kstring_size(name));

    TValue *node = klispH_setstr(K, tv2table(K->require_table), 
                                 tv2str(saved_name));
    
    /* throw error if already registered */
    if (!ttisfree(*node)) {
        klispE_throw_simple_with_irritants(K, "Name already registered", 
                                           1, name);
        return;
    }

    *node = KTRUE;
    kapply_cc(K, KINERT);
}

void unregister_requirementB(klisp_State *K)
{
    bind_1tp(K, K->next_value, "string", ttisstring, name);
    if (kstring_emptyp(name)) {
        klispE_throw_simple(K, "Empty name");
        return;
    }
    TValue saved_name = kstring_immutablep(name)? name :
        kstring_new_bs_imm(K, kstring_buf(name), kstring_size(name));

    TValue *node = klispH_setstr(K, tv2table(K->require_table), 
                                 tv2str(saved_name));

    /* throw error if not registered */
    if (ttisfree(*node)) {
        klispE_throw_simple_with_irritants(K, "Unregistered name", 1, name);
        return;
    }

    *node = KFREE;
    kapply_cc(K, KINERT);
}

/* will throw an error if not found */
void find_required_filename(klisp_State *K)
{
    bind_1tp(K, K->next_value, "string", ttisstring, name);
    if (kstring_emptyp(name)) {
        klispE_throw_simple(K, "Empty name");
        return;
    }
    TValue filename = find_file(K, name, K->require_path);
    
    if (kstring_emptyp(filename)) {
        klispE_throw_simple_with_irritants(K, "Not found", 1, name);
        return;
    }
    kapply_cc(K, filename);
}

/* 15.2.3 get-module */
void get_module(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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

    TValue ret_env_cont = kmake_continuation(K, kget_cc(K), do_return_value, 
                                             1, env);
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
void display(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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

void read_line(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

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

    TValue obj = kread_line_from_port(K, port);
    kapply_cc(K, obj);
}

/* 15.1.? flush-output-port */
void flush(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
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
       like in r5rs? */
    add_applicative(K, ground_env, "close-input-file", close_file, 1,
                    b2tv(false));
    add_applicative(K, ground_env, "close-output-file", close_file, 1,
                    b2tv(true));
    /* 15.1.? Use the r7rs names, this has more sense in the face of 
       the different port types available in klisp */
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
    /* 15.1.? write-simple */
    add_applicative(K, ground_env, "write-simple", gwrite_simple, 0);

    /* 15.1.? eof-object? */
    add_applicative(K, ground_env, "eof-object?", typep, 2, symbol, 
                    i2tv(K_TEOF));
    /* 15.1.? newline */
    add_applicative(K, ground_env, "newline", newline, 0);
    /* 15.1.? display */
    add_applicative(K, ground_env, "display", display, 0);
    /* 15.1.? read-line */
    add_applicative(K, ground_env, "read-line", read_line, 0);
    /* 15.1.? flush-output-port */
    add_applicative(K, ground_env, "flush-output-port", flush, 0);

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
    /* 15.2.? require */
    add_applicative(K, ground_env, "require", require, 0);
    /* 15.2.? registered-requirement? */
    add_applicative(K, ground_env, "registered-requirement?", 
                    registered_requirementP, 0);
    /* 15.2.? register-requirement! */
    add_applicative(K, ground_env, "register-requirement!", 
                    register_requirementB, 0);
    /* 15.2.? unregister-requirement! */
    add_applicative(K, ground_env, "unregister-requirement!", 
                    unregister_requirementB, 0);
    /* 15.2.? find-required-filename */
    add_applicative(K, ground_env, "find-required-filename", 
                    find_required_filename, 0);
    /* 15.2.3 get-module */
    add_applicative(K, ground_env, "get-module", get_module, 0);

    /*
     * That's all there is in the report combined with r5rs and r7rs scheme.
     * TODO
     * It would be good to be able to select between append, truncate and
     * error if a file exists, but that would need to be an option in all three 
     * methods of opening. Also some directory checking, traversing, etc,
     * would be nice
     */
}

/* init continuation names */
void kinit_ports_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);

    add_cont_name(K, t, do_close_file_ret, "close-file-and-ret");
}
