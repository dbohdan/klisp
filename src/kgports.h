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
/* TODO */

/* 15.1.5 open-input-file, open-output-file */
void open_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.1.6 close-input-file, close-output-file */
void close_file(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 15.2.1 call-with-input-file, call-with-output-file */
/* TODO */

/* 15.2.2 load */
/* TODO */

/* 15.2.3 get-module */
/* TODO */

#endif
