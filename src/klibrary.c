/*
** klibrary.c
** Kernel Libraries
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "klibrary.h"
#include "kmem.h"
#include "kgc.h"

/* GC: Assumes env & ext_list are roooted */
/* ext_list should be immutable (and it may be empty) */
TValue kmake_library(klisp_State *K, TValue env, TValue exp_list)
{
    klisp_assert(ttisnil(exp_list) || kis_immutable(exp_list));
    Library *new_lib = klispM_new(K, Library);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_lib, K_TLIBRARY, 
                K_FLAG_CAN_HAVE_NAME);

    /* library specific fields */
    new_lib->env = env;
    new_lib->exp_list = exp_list;
    return gc2lib(new_lib);
}
