/*
** kmodule.c
** Kernel Modules
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kmodule.h"
#include "kmem.h"
#include "kgc.h"

/* GC: Assumes env & ext_list are roooted */
/* ext_list should be immutable */
TValue kmake_module(klisp_State *K, TValue env, TValue exp_list)
{
    klisp_assert(kis_immutable(exp_list));
    Module *new_mod = klispM_new(K, Module);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_mod, K_TMODULE, 
                K_FLAG_CAN_HAVE_NAME);

    /* module specific fields */
    new_mod->env = env;
    new_mod->exp_list = exp_list;
    return gc2mod(new_mod);
}
