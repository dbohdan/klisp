/*
** kapplicative.h
** Kernel Applicatives
** See Copyright Notice in klisp.h
*/

#ifndef kapplicative_h
#define kapplicative_h

#include "kobject.h"
#include "kstate.h"
#include "koperative.h"

/* GC: Assumes underlying is rooted */
TValue kwrap(klisp_State *K, TValue underlying);

/* GC: Assumes all argps are rooted */
#define kmake_applicative(K_, ...)                      \
    ({ klisp_State *K__ = (K_);                         \
        TValue op = kmake_operative(K__, __VA_ARGS__);	\
        krooted_tvs_push(K__, op);                      \
        TValue app = kwrap(K__, op);                    \
        krooted_tvs_pop(K__);                           \
        (app); })

inline TValue kunwrap(TValue app) { return (tv2app(app)->underlying); }

#endif
