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

/* should be at least < GC_PROTECT_SIZE - 3 */
#define CONT_MAX_ARGS 16 

TValue kmake_continuation(klisp_State *K, TValue parent, TValue name, 
			  TValue si, klisp_Cfunc fn, int32_t xcount, ...)
{
    va_list argp;

    klisp_assert(xcount < CONT_MAX_ARGS);

    TValue args[CONT_MAX_ARGS];
    va_start(argp, xcount);
    for (int i = 0; i < xcount; i++) {
	TValue val = va_arg(argp, TValue);
	krooted_tvs_push(K, val);
	args[i] = val;
    }
    va_end(argp);

    krooted_tvs_push(K, parent);
    krooted_tvs_push(K, name);
    krooted_tvs_push(K, si);

    Continuation *new_cont = (Continuation *)
	klispM_malloc(K, sizeof(Continuation) + sizeof(TValue) * xcount);


    for (int i = 0; i < xcount; i++) {
	TValue val = args[i];
	new_cont->extra[i] = val;
	krooted_tvs_pop(K);
    }

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_cont, K_TCONTINUATION, 0);

    /* continuation specific fields */
    new_cont->mark = KFALSE;    
    new_cont->name = name;
    new_cont->si = si;
    new_cont->parent = parent;
    new_cont->fn = fn;
    new_cont->extra_size = xcount;
    /* new_cont->extra was already set */

    return gc2cont(new_cont);
}
