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

/*
** Simple read/write loop
*/
void main_body()
{
    TValue obj = KNIL;

    while(!ttiseof(obj)) {
	obj = kread();
	kwrite(obj);
	knewline();
    }
}

int main(int argc, char *argv[]) 
{
    printf("Read/Write Test\n");

    /* TEMP: old initialization */
    kread_file = stdin;
    kread_filename = "*STDIN*";
    kwrite_file = stdout;
    kread_init();
    kwrite_init();

    klisp_State *K = klispL_newstate();
    int ret_value = 0;
    bool done = false;

    while(!done) {
	if (setjmp(K->error_jb)) {
	    /* error signaled */
	    if (!K->error_can_cont) {
		ret_value = 1;
		done = true;
	    }
	} else {
	    main_body();
	    ret_value = 0;
	    done = true;
	}
    }

    klisp_close(K);
    return ret_value;
}
