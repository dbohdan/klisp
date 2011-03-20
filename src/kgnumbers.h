/*
** kgnumbers.h
** Numbers features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgnumbers_h
#define kgnumbers_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 15.5.1 number?, finite?, integer? */
/* use ftypep & ftypep_predp */

/* Helpers for typed predicates */
/* XXX: this should probably be in a file knumber.h but there is no real need for 
   that file yet */
bool knumberp(TValue obj);
bool kfinitep(TValue obj);
bool kintegerp(TValue obj);


/* 12.5.2 =? */
/* uses typed_bpredp */

/* 12.5.3 <?, <=?, >?, >=? */
/* use typed_bpredp */

/* Helpers for typed binary predicates */
/* XXX: this should probably be in a file knumber.h but there is no real need for 
   that file yet */
bool knum_eqp(TValue n1, TValue n2);
bool knum_ltp(TValue n1, TValue n2);
bool knum_lep(TValue n1, TValue n2);
bool knum_gtp(TValue n1, TValue n2);
bool knum_gep(TValue n1, TValue n2);

/* 12.5.4 + */
/* TEMP: for now only accept two arguments */
void kplus(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.5.5 * */
/* TEMP: for now only accept two arguments */
void ktimes(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.5.6 - */
/* TEMP: for now only accept two arguments */
void kminus(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.5.7 zero? */
/* uses ftyped_predp */

/* Helper for zero? */
bool kzerop(TValue n);

/* 12.5.8 div, mod, div-and-mod */
/* TODO */

/* 12.5.9 div0, mod0, div0-and-mod0 */
/* TODO */

/* 12.5.10 positive?, negative? */
/* use ftyped_predp */

/* 12.5.10 odd?, even? */
/* use ftyped_predp */

/* Helpers for positive?, negative?, odd? & even? */
bool kpositivep(TValue n);
bool knegativep(TValue n);
bool koddp(TValue n);
bool kevenp(TValue n);

/* Misc Helpers */
inline bool kfast_zerop(TValue n) { return ttisfixint(n) && ivalue(n) == 0; }
/* TEMP: only exact infinties */
inline TValue kneg_inf(TValue i) 
{ 
    return tv_equal(i, KEPINF)? KEMINF : KEPINF; 
}

#endif
