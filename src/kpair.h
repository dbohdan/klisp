/*
** kpair.h
** Kernel Pairs
** See Copyright Notice in klisp.h
*/

#ifndef kpair_h
#define kpair_h

#include "kobject.h"

/* TODO: add type assertions */
/* TODO: add more kc[ad]*r combinations */
#define kcar(p_) (((Pair *)(p_.tv.v.gc))->car)
#define kcdr(p_) (((Pair *)(p_.tv.v.gc))->cdr)

#define kset_car(p_, v_) (kcar(p_) = v_)
#define kset_cdr(p_, v_) (kcdr(p_) = v_)

#define kdummy_cons (kcons(KNIL, KNIL))

/* XXX: for now all pairs are mutable */
TValue kcons(TValue, TValue);

#endif
