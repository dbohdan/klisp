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

/* 15.2.1 call-with-input-file, call-with-output-file */
/* TODO */

/* 15.2.2 load */
/* TODO */

/* 15.2.3 get-module */
/* TODO */
