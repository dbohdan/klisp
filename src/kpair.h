/*
** kpair.h
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

#ifndef kpair_h
#define kpair_h

#include "kobject.h"
#include "kstate.h"
#include "klimits.h"
#include "kgc.h"

/* can't be inlined... */
bool kpairp(TValue obj);
bool kimmutable_pairp(TValue obj);
bool kmutable_pairp(TValue obj);

static inline TValue kcar(TValue p)
{
    klisp_assert(kpairp(p));
    return tv2pair(p)->car;
}

static inline TValue kcdr(TValue p)
{
    klisp_assert(kpairp(p));
    return tv2pair(p)->cdr;
}

#define kcaar(p_) (kcar(kcar(p_)))
#define kcadr(p_) (kcar(kcdr(p_)))
#define kcdar(p_) (kcdr(kcar(p_)))
#define kcddr(p_) (kcdr(kcdr(p_)))

#define kcaaar(p_) (kcar(kcar(kcar(p_))))
#define kcaadr(p_) (kcar(kcar(kcdr(p_))))
#define kcadar(p_) (kcar(kcdr(kcar(p_))))
#define kcaddr(p_) (kcar(kcdr(kcdr(p_))))
#define kcdaar(p_) (kcdr(kcar(kcar(p_))))
#define kcdadr(p_) (kcdr(kcar(kcdr(p_))))
#define kcddar(p_) (kcdr(kcdr(kcar(p_))))
#define kcdddr(p_) (kcdr(kcdr(kcdr(p_))))

#define kcaaaar(p_) (kcar(kcar(kcar(kcar(p_)))))
#define kcaaadr(p_) (kcar(kcar(kcar(kcdr(p_)))))
#define kcaadar(p_) (kcar(kcar(kcdr(kcar(p_)))))
#define kcaaddr(p_) (kcar(kcar(kcdr(kcdr(p_)))))
#define kcadaar(p_) (kcar(kcdr(kcar(kcar(p_)))))
#define kcadadr(p_) (kcar(kcdr(kcar(kcdr(p_)))))
#define kcaddar(p_) (kcar(kcdr(kcdr(kcar(p_)))))
#define kcadddr(p_) (kcar(kcdr(kcdr(kcdr(p_)))))

#define kcdaaar(p_) (kcdr(kcar(kcar(kcar(p_)))))
#define kcdaadr(p_) (kcdr(kcar(kcar(kcdr(p_)))))
#define kcdadar(p_) (kcdr(kcar(kcdr(kcar(p_)))))
#define kcdaddr(p_) (kcdr(kcar(kcdr(kcdr(p_)))))
#define kcddaar(p_) (kcdr(kcdr(kcar(kcar(p_)))))
#define kcddadr(p_) (kcdr(kcdr(kcar(kcdr(p_)))))
#define kcdddar(p_) (kcdr(kcdr(kcdr(kcar(p_)))))
#define kcddddr(p_) (kcdr(kcdr(kcdr(kcdr(p_)))))

inline void kset_car(TValue p, TValue v)
{
    klisp_assert(kmutable_pairp(p));
    tv2pair(p)->car = v;
}

inline void kset_cdr(TValue p, TValue v)
{
    klisp_assert(kmutable_pairp(p));
    tv2pair(p)->cdr = v;
}

/* These two are the same but can write immutable pairs,
   use with care */
inline void kset_car_unsafe(klisp_State *K, TValue p, TValue v)
{
    klisp_assert(kpairp(p));
    UNUSED(K);
/*    klispC_barrier(K, gcvalue(p), v); */
    tv2pair(p)->car = v;
}

inline void kset_cdr_unsafe(klisp_State *K, TValue p, TValue v)
{
    klisp_assert(kpairp(p));
    UNUSED(K);
/*    klispC_barrier(K, gcvalue(p), v); */
    tv2pair(p)->cdr = v;
}

/* GC: assumes car & cdr are rooted */
TValue kcons_g(klisp_State *K, bool m, TValue car, TValue cdr);

/* GC: assumes all argps are rooted */
TValue klist_g(klisp_State *K, bool m, int32_t n, ...);

#define kcons(K_, car_, cdr_) (kcons_g(K_, true, car_, cdr_))
#define kimm_cons(K_, car_, cdr_) (kcons_g(K_, false, car_, cdr_))
#define klist(K_, n_, ...) (klist_g(K_, true, n_, __VA_ARGS__))
#define kimm_list(K_, n_, ...) (klist_g(K_, false, n_, __VA_ARGS__))

#endif
