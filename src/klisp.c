/*
** klisp.c
** Kernel stand-alone interpreter
** See Copyright Notice in klisp.h
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <setjmp.h>

/* turn on assertions for internal checking */
#define klisp_assert (assert)
#include "klimits.h"

#include "klisp.h"
#include "kstate.h"
#include "kauxlib.h"

int main(int argc, char *argv[]) 
{
    printf("REPL Test\n");

    klisp_State *K = klispL_newstate();
    klispS_init_repl(K);
    klispS_run(K);
    klisp_close(K);

    printf("Done!\n");
    return 0;
}
