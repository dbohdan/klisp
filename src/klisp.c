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
#include "kobject.h"
#include "kauxlib.h"
#include "kstate.h"
#include "kread.h"
#include "kwrite.h"
#include "keval.h"
#include "krepl.h"

#include "kcontinuation.h"
#include "kenvironment.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kpair.h"
#include "ksymbol.h"
#include "kerror.h"

int main(int argc, char *argv[]) 
{
    printf("Read/Write Test\n");

    klisp_State *K = klispL_newstate();
    kinit_repl(K);

    int ret_value = 0;
    bool done = false;

    while(!done) {
	if (setjmp(K->error_jb)) {
	    /* continuation called */
	    /* TEMP: do nothing, the loop will call the continuation */
	} else {
	    /* all ok, continue with next func */
	    while (K->next_func) {
		if (ttisnil(K->next_env)) {
		    /* continuation application */
		    klisp_Cfunc fn = (klisp_Cfunc) K->next_func;
		    (*fn)(K, K->next_xparams, K->next_value);
		} else {
		    /* operative calling */
		    klisp_Ofunc fn = (klisp_Ofunc) K->next_func;
		    (*fn)(K, K->next_xparams, K->next_value, K->next_env);
		}
	    }
	    printf("Done!\n");
	    ret_value = 0;
	    done = true;
	}
    }

    klisp_close(K);
    return ret_value;
}
