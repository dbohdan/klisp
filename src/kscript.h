/*
** krepl.h
** klisp noninteractive script execution
** See Copyright Notice in klisp.h
*/

#ifndef kscript_h
#define kscript_h

#include <stdio.h>
#include "klisp.h"
#include "kstate.h"
#include "kobject.h"

void kinit_script(klisp_State *K, int argc, char *argv[]);

/* continuation functions */
void do_script_exit(klisp_State *K, TValue *xparams, TValue obj);
void do_script_error(klisp_State *K, TValue *xparams, TValue obj);

/* default exit code in case of error according to SRFI-22 */

#define KSCRIPT_DEFAULT_ERROR_EXIT_CODE   70

#endif
