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
/* TODO: add more kc[ad]*r combinations */
#define kcar(p_) (tv2pair(p_)->car)
#define kcdr(p_) (tv2pair(p_)->cdr)

#define kset_car(p_, v_) (kcar(p_) = (v_))
#define kset_cdr(p_, v_) (kcdr(p_) = (v_))

#define kdummy_cons(st_) (kcons(st_, KNIL, KNIL))

/* TEMP: for now all pairs are mutable */
TValue kcons(klisp_State *K, TValue car, TValue cdr);

#define kget_source_info(p_) (tv2pair(p_)->si)
#define kset_source_info(p_, si_) (kget_source_info(p_) = (si_))

#endif
