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

/* 15.1.3 with-input-from-file, with-ouput-to-file */
/* TODO */

/* 15.1.4 get-current-input-port, get-current-output-port */
void get_current_port(klisp_State *K, TValue *xparams, TValue ptree,
		      TValue denv);

/* 15.1.5 open-input-file, open-output-file */
void open_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.6 close-input-file, close-output-file */
void close_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.7 read */
void read(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.8 write */
void write(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.? eof-object? */
/* uses typep */

/* 15.1.? newline */
void newline(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.2.1 call-with-input-file, call-with-output-file */
void call_with_file(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 15.2.2 load */
void load(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.2.3 get-module */
void get_module(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

#endif
