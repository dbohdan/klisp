/*
** kenvironmment.c
** Kernel Environments
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "kenvironment.h"
#include "kpair.h"
#include "ksymbol.h"
#include "kobject.h"
#include "kerror.h"
#include "kstate.h"
#include "kmem.h"

/* TEMP: for now allow only a single parent */
TValue kmake_environment(klisp_State *K, TValue parent)
{
    Environment *new_env = klispM_new(K, Environment);

    new_env->next = NULL;
    new_env->gct = 0;
    new_env->tt = K_TENVIRONMENT;
    new_env->mark = KFALSE;    
    new_env->parents = parent;
    /* TEMP: for now the bindings are an alist */
    new_env->bindings = KNIL;

    return gc2env(new_env);
}

/* 
** Helper function for kadd_binding and kget_binding,
** returns KNIL or a pair with sym as car.
*/
TValue kfind_local_binding(klisp_State *K, TValue bindings, TValue sym)
{
    /* avoid warnings */
    (void) K;

    while(!ttisnil(bindings)) {
	TValue first = kcar(bindings);
	TValue first_sym = kcar(first);
	if (tv_equal(sym, first_sym))
	    return first;
	bindings = kcdr(bindings);
    }
    return KNIL;
}

/*
** Some helper macros
*/
#define kenv_parents(kst_, env_) (tv2env(env_)->parents)
#define kenv_bindings(kst_, env_) (tv2env(env_)->bindings)

void kadd_binding(klisp_State *K, TValue env, TValue sym, TValue val)
{
    TValue oldb = kfind_local_binding(K, kenv_parents(K, env), sym);

    if (ttisnil(oldb)) {
	/* XXX: unrooted pair */
	TValue new_pair = kcons(K, sym, val);
	kenv_bindings(K, env) = kcons(K, new_pair, kenv_bindings(K, env));
    } else {
	kset_cdr(oldb, val);
    }
}

TValue kget_binding(klisp_State *K, TValue env, TValue sym)
{
    while(!ttisnil(env)) {
	TValue oldb = kfind_local_binding(K, kenv_parents(K, env), sym);
	if (!ttisnil(oldb))
	    return kcdr(oldb);
	env = kenv_parents(K, env);
    }
    klispE_throw(K, strcat("Unbound symbol: ", ksymbol_buf(sym)), true);
    /* avoid warning */
    return KINERT;
}
