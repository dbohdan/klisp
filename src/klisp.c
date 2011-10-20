/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <setjmp.h>

#include "klimits.h"

#include "klisp.h"
#include "kstate.h"
#include "kauxlib.h"
#include "kscript.h"

int main(int argc, char *argv[]) 
{
    if (argc <= 1) {
        klisp_State *K = klispL_newstate();
        klispS_init_repl(K);
        klispS_run(K);
        klisp_close(K);
        return 0;
    } else {
        klisp_State *K = klispL_newstate();
        kinit_script(K, argc - 1, argv + 1);
        klispS_run(K);
        int exit_code = K->script_exit_code;
        klisp_close(K);
        return exit_code;
    }
}
