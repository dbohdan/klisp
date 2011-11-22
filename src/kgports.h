/*
** kgports.h
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgports_h
#define kgports_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

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

/* 15.1.3 with-input-from-file, with-ouput-to-file */
/* 15.1.? with-error-to-file */
void with_file(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 15.1.4 get-current-input-port, get-current-output-port */
/* 15.1.? get-current-error-port */
void get_current_port(klisp_State *K, TValue *xparams, TValue ptree,
		      TValue denv);

/* 15.1.5 open-input-file, open-output-file */
void open_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? open-input-string, open-output-string */
/* 15.1.? open-input-bytevector, open-output-bytevector */
void open_mport(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.6 close-input-file, close-output-file */
void close_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? close-port, close-input-port, close-output-port */
void close_port(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? get-output-string, get-output-bytevector */
void get_output_buffer(klisp_State *K, TValue *xparams, TValue ptree, 
		       TValue denv);

/* 15.1.7 read */
void gread(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.8 write */
void gwrite(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? eof-object? */
/* uses typep */

/* 15.1.? newline */
void newline(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? write-char */
void write_char(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper for read-char and peek-char */
void read_peek_char(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 15.1.? read-char */
/* uses read_peek_char */

/* 15.1.? peek-char */
/* uses read_peek_char */

/* 15.1.? char-ready? */
/* XXX: this always return #t, proper behaviour requires platform 
   specific code (probably select for posix, a thread for windows
   (at least for files & consoles), I think pipes and sockets may
   have something */
void char_readyp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.2.1 call-with-input-file, call-with-output-file */
void call_with_file(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 15.2.2 load */
void load(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.2.3 get-module */
void get_module(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.2.? display */
void display(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

void do_close_file_ret(klisp_State *K);

/* 15.1.? flush-output-port */
void flush(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? file-exists? */
void file_existsp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? delete-file */
void delete_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? rename-file */
void rename_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* init ground */
void kinit_ports_ground_env(klisp_State *K);

#endif
