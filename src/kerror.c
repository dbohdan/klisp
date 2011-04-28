
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "klisp.h"
#include "kpair.h"
#include "kstate.h"
#include "kmem.h"
#include "kstring.h"

/* TODO: check that all objects passed to throw are rooted */

/* GC: assumes all objs passed are rooted */
TValue klispE_new(klisp_State *K, TValue who, TValue cont, TValue msg, 
		  TValue irritants) 
{
    Error *new_error = klispM_new(K, Error);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_error, K_TERROR, 0);

    /* error specific fields */
    new_error->who = who;
    new_error->cont = cont;
    new_error->msg = msg;
    new_error->irritants = irritants;

    return gc2error(new_error);
}

/*
** Clear all stacks & buffers 
*/
void clear_buffers(klisp_State *K)
{
    /* These shouldn't cause GC, but just in case do them first,
     an object may be protected in tvs or vars */
    ks_sclear(K);
    ks_tbclear(K);
    K->shared_dict = KNIL;

    UNUSED(kcutoff_dummy1(K));
    UNUSED(kcutoff_dummy2(K));
    UNUSED(kcutoff_dummy3(K));

    krooted_tvs_clear(K);
    krooted_vars_clear(K);
}

/*
** Throw a simple error obj with:
** {
**     who: current operative/continuation, 
**     cont: current continuation, 
**     message: msg, 
**     irritants: ()
** }
*/
/* GC: assumes all objs passed are rooted */
void klispE_throw_simple(klisp_State *K, char *msg)
{
    TValue error_msg = kstring_new_b_imm(K, msg);
    krooted_tvs_push(K, error_msg);
    TValue error_obj = 
	klispE_new(K, K->next_obj, K->curr_cont, error_msg, KNIL);
     /* clear buffer shouldn't cause GC, but just in case... */
    krooted_tvs_push(K, error_obj);
    clear_buffers(K); /* this pops both error_msg & error_obj */
    /* call_cont protects error from gc */
    kcall_cont(K, K->error_cont, error_obj);
}

/*
** Throw an error obj with:
** {
**     who: current operative/continuation, 
**     cont: current continuation, 
**     message: msg, 
**     irritants: irritants
** }
*/
/* GC: assumes all objs passed are rooted */
void klispE_throw_with_irritants(klisp_State *K, char *msg, TValue irritants)
{
    TValue error_msg = kstring_new_b_imm(K, msg);
    krooted_tvs_push(K, error_msg);
    TValue error_obj = 
	klispE_new(K, K->next_obj, K->curr_cont, error_msg, irritants);
     /* clear buffer shouldn't cause GC, but just in case... */
    krooted_tvs_push(K, error_obj);
    clear_buffers(K); /* this pops both error_msg & error_obj */
    /* call_cont protects error from gc */
    kcall_cont(K, K->error_cont, error_obj);
}
