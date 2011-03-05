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

#include "kcontinuation.h"
#include "kenvironment.h"
#include "koperative.h"

/*
** Simple read/write loop
*/
void main_body(klisp_State *K)
{
    TValue obj = KNIL;

    while(!ttiseof(obj)) {
	obj = kread(K);
	kwrite(K, obj);
	knewline(K);
    }
}

/* the exit continuation, it exits the loop */
void exit_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* avoid warnings */
    (void) xparams;
    (void) obj;

    /* force the loop to terminate */
    K->next_func = NULL;
    return;
}

/* the underlying function of the read operative */
void read_fn(klisp_State *K, TValue *xparams, TValue ptree, TValue env)
{
    (void) ptree;
    (void) env;
    (void) xparams;
    TValue obj = kread(K);
    kapply_cc(K, obj);
}

/* the underlying function of the loop */
void loop_fn(klisp_State *K, TValue *xparams, TValue obj)
{
    /* tparams[0] is the read operative,
     in tparams[1] a dummy environment */
    if (ttiseof(obj)) {
	/* this will in turn call main_cont */
	kapply_cc(K, obj);
    } else {
	kwrite(K, obj);
	knewline(K);
	TValue read_op = *xparams;
	TValue dummy_env = *xparams;
	TValue new_cont = kmake_continuation(
	    K, kget_cc(K), KNIL, KNIL, &loop_fn, 2, read_op, dummy_env);
	kset_cc(K, new_cont);
	ktail_call(K, read_op, KNIL, dummy_env);
    }
} 

int main(int argc, char *argv[]) 
{
    printf("Read/Write Test\n");

    klisp_State *K = klispL_newstate();

    /* set up the continuations */
    TValue read_op = kmake_operative(K, KNIL, KNIL, read_fn, 0);
    TValue dummy_env = kmake_empty_environment(K);
    TValue root_cont = kmake_continuation(K, KNIL, KNIL, KNIL,
					  exit_fn, 0);
    TValue loop_cont = kmake_continuation(K, root_cont, KNIL, KNIL,
					  loop_fn, 2, read_op, dummy_env);
    kset_cc(K, loop_cont);
    /* NOTE: this will take effect only in the while (K->next_func) loop */
    klispS_tail_call(K, read_op, KNIL, dummy_env);

    int ret_value = 0;
    bool done = false;

    while(!done) {
	if (setjmp(K->error_jb)) {
	    /* error signaled */
	    if (K->error_can_cont) {
		/* XXX: clear stack and char buffer, clear shared dict */
		/* TODO: put these in handlers for read-token, read and write */
		ks_sclear(K);
		ks_tbclear(K);
		K->shared_dict = KNIL;
	    } else {
		printf("Aborting...\n");
		ret_value = 1;
		done = true;
	    }
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
