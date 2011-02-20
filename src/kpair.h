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
#define kcar(p_) (tv2pair(p_)->car)
#define kcdr(p_) (tv2pair(p_)->cdr)

#define kset_car(p_, v_) (kcar(p_) = v_)
#define kset_cdr(p_, v_) (kcdr(p_) = v_)

#define kdummy_cons() (kcons(KNIL, KNIL))

/* TEMP: for now all pairs are mutable */
TValue kcons(TValue, TValue);

#define kget_source_info(p_) (tv2pair(p_)->si)
#define kset_source_info(p_, si_) (kget_source_info(p_) = si_)

#endif
