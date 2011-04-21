/*
** koperative.c
** Kernel Operatives
** See Copyright Notice in klisp.h
*/

#include <stdarg.h>

#include "koperative.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* GC: Assumes all argps are rooted */
TValue kmake_operative(klisp_State *K, klisp_Ofunc fn, int32_t xcount, ...)
{
    va_list argp;

    Operative *new_op = (Operative *) 
	klispM_malloc(K, sizeof(Operative) + sizeof(TValue) * xcount);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_op, K_TOPERATIVE, 
		K_FLAG_CAN_HAVE_NAME);

    /* operative specific fields */
    new_op->fn = fn;
    new_op->extra_size = xcount;

    va_start(argp, xcount);
    for (int i = 0; i < xcount; i++) {
	new_op->extra[i] = va_arg(argp, TValue);
    }
    va_end(argp);

    return gc2op(new_op);
}
