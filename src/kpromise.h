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

TValue kmake_promise(klisp_State *K, TValue name, TValue si,
		     TValue exp, TValue maybe_env);

#define kpromise_node(p_) (tv2prom(p_)->node)
inline void kdetermine_promise(TValue p, TValue obj)
{
    TValue node = kpromise_node(p);
    kset_car(node, obj);
    kset_cdr(node, KNIL);
}

#endif
