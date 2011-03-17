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
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "ksymbol.h"
#include "kread.h"
#include "kwrite.h"

#include "kghelpers.h"
#include "kgports.h"

/* 15.1.1 port? */
/* uses typep */

/* 15.1.2 input-port?, output-port? */
/* use ftypep */

/* 15.1.3 with-input-from-file, with-ouput-to-file */
/* TODO */

/* 15.1.4 get-current-input-port, get-current-output-port */
/* TODO */

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
/* TEMP: the port parameter is not optional yet */
void read(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, "read", ptree, "port", ttisport, port);
    
    if (!kport_is_input(port)) {
	klispE_throw(K, "read: the port should be an input port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw(K, "read: the port is already closed");
	return;
    }

    /* TEMP: for now set this by hand */
    K->curr_in = tv2port(port)->file;
    ktok_reset_source_info(K); /* this should be saved in the port
				  and restored before the call to 
				  read and saved after it */
    TValue obj = kread(K); /* this may throw an error, that's ok */
    kapply_cc(K, obj);
}

/* 15.1.8 write */
/* TEMP: the port parameter is not optional yet */
void write(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_2tp(K, "write", ptree, "any", anytype, obj,
	     "port", ttisport, port);

    if (!kport_is_output(port)) {
	klispE_throw(K, "write: the port should be an output port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw(K, "write: the port is already closed");
	return;
    }
    
    /* TEMP: for now set this by hand */
    K->curr_out = tv2port(port)->file;

    kwrite(K, obj);
    kapply_cc(K, KINERT);
}

/* 15.1.? eof-object? */
/* uses typep */

/* 15.1.? newline */
/* TEMP: the port parameter is not optional yet */
void newline(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, "newline", ptree, "port", ttisport, port);

    if (!kport_is_output(port)) {
	klispE_throw(K, "write: the port should be an output port");
	return;
    } else if (kport_is_closed(port)) {
	klispE_throw(K, "write: the port is already closed");
	return;
    }
    
    /* TEMP: for now set this by hand */
    K->curr_out = tv2port(port)->file;

    knewline(K);
    kapply_cc(K, KINERT);
}

/* 15.2.1 call-with-input-file, call-with-output-file */
/* TODO */

/* 15.2.2 load */
/* TODO */

/* 15.2.3 get-module */
/* TODO */
