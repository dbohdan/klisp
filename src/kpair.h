/*
** kpair.h
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

#ifndef kpair_h
#define kpair_h

#include "kobject.h"
#include "kstate.h"

/* TODO: add type assertions */
#define kcar(p_) (tv2pair(p_)->car)
#define kcdr(p_) (tv2pair(p_)->cdr)

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

/* these will also work with immutable pairs */
#define kset_car(p_, v_) (kcar(p_) = (v_))
#define kset_cdr(p_, v_) (kcdr(p_) = (v_))

#define kdummy_cons(st_) (kcons(st_, KNIL, KNIL))
#define kdummy_imm_cons(st_) (kimm_cons(st_, KNIL, KNIL))

TValue kcons_g(klisp_State *K, bool m, TValue car, TValue cdr);

#define kcons(K_, car_, cdr_) (kcons_g(K_, true, car_, cdr_))
#define kimm_cons(K_, car_, cdr_) (kcons_g(K_, false, car_, cdr_))

#define kget_source_info(p_) (tv2pair(p_)->si)
#define kset_source_info(p_, si_) (kget_source_info(p_) = (si_))

bool kpairp(TValue obj);

#endif
