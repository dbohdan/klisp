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

TValue kmake_continuation(klisp_State *K, TValue parent, TValue name, 
			  TValue si, klisp_Cfunc fn, int32_t xcount, ...)
{
    va_list argp;
    Continuation *new_cont = (Continuation *)
	klispM_malloc(K, sizeof(Continuation) + sizeof(TValue) * xcount);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_cont, K_TCONTINUATION, 0);

    /* continuation specific fields */
    new_cont->mark = KFALSE;    
    new_cont->name = name;
    new_cont->si = si;
    new_cont->parent = parent;
    new_cont->fn = fn;
    new_cont->extra_size = xcount;

    va_start(argp, xcount);
    for (int i = 0; i < xcount; i++) {
	new_cont->extra[i] = va_arg(argp, TValue);
    }
    va_end(argp);
    return gc2cont(new_cont);
}
