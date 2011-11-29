/*
** kcontinuation.c
** Kernel Continuations
** See Copyright Notice in klisp.h
*/

#include <stdarg.h>

#include "kcontinuation.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

TValue kmake_continuation(klisp_State *K, TValue parent, klisp_CFunction fn, 
			  int32_t xcount, ...)
{
    va_list argp;

    Continuation *new_cont = (Continuation *)
	klispM_malloc(K, sizeof(Continuation) + sizeof(TValue) * xcount);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_cont, K_TCONTINUATION, 
		K_FLAG_CAN_HAVE_NAME);


    /* continuation specific fields */
    new_cont->mark = KFALSE;    
    new_cont->parent = parent;

    TValue comb = K->next_obj;
    if (ttiscontinuation(comb))
	comb = tv2cont(comb)->comb;
    new_cont->comb = comb;

    new_cont->fn = fn;
    new_cont->extra_size = xcount;

    va_start(argp, xcount);
    for (int i = 0; i < xcount; i++) {
	new_cont->extra[i] = va_arg(argp, TValue);
    }
    va_end(argp);

    TValue res = gc2cont(new_cont);
    /* Add the current source info as source info (may be changed later) */
    /* TODO: find all the places where this should be changed (like $and?, 
       $sequence), and change it */
    kset_source_info(K, res, kget_csi(K));
    return res;
}
