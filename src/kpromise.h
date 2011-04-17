/*
** kpromise.h
** Kernel Promises
** See Copyright Notice in klisp.h
*/

#ifndef kpromise_h
#define kpromise_h

#include "kobject.h"
#include "kstate.h"
#include "kpair.h"

/* GC: Assumes exp & maybe_env are roooted */
TValue kmake_promise(klisp_State *K, TValue exp, TValue maybe_env);

#define kpromise_node(p_) (tv2prom(p_)->node)
#define kpromise_exp(p_) (kcar(kpromise_node(p_)))
#define kpromise_maybe_env(p_) (kcdr(kpromise_node(p_)))

#endif
