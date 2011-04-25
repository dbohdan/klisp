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
bool krationalp(TValue obj);
bool krealp(TValue obj);


/* 12.5.2 =? */
/* uses typed_bpredp */

/* 12.5.3 <?, <=?, >?, >=? */
/* use typed_bpredp */

/* Helpers for typed binary predicates */
/* XXX: this should probably be in a file knumber.h but there is no real need for 
   that file yet */
bool knum_eqp(klisp_State *K, TValue n1, TValue n2);
bool knum_ltp(klisp_State *K, TValue n1, TValue n2);
bool knum_lep(klisp_State *K, TValue n1, TValue n2);
bool knum_gtp(klisp_State *K, TValue n1, TValue n2);
bool knum_gep(klisp_State *K, TValue n1, TValue n2);

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

/* 12.5.11 odd?, even? */
/* use ftyped_predp */

/* Helpers for positive?, negative?, odd? & even? */
bool kpositivep(TValue n);
bool knegativep(TValue n);
bool koddp(TValue n);
bool kevenp(TValue n);

/* 12.5.8 div, mod, div-and-mod */
/* use div_mod */

/* 12.5.9 div0, mod0, div0-and-mod0 */
/* use div_mod */

/* Helper for div and mod */
#define FDIV_DIV 1
#define FDIV_MOD 2
#define FDIV_ZERO 4

void kdiv_mod(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);


/* 12.5.12 abs */
void kabs(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.5.13 min, max */
/* use kmin_max */

/* Helper */
#define FMIN (true)
#define FMAX (false)
void kmin_max(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.5.14 gcm, lcm */
void kgcd(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);
void klcm(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.8.1 rational? */
/* uses ftypep */

/* 12.8.2 / */
void kdivided(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.8.3 numerator, denominator */
void knumerator(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);
void kdenominator(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 12.8.4 floor, ceiling, truncate, round */
void kreal_to_integer(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* 12.8.5 rationalize, simplest-rational */
/* TODO */

/* REFACTOR: These should be in a knumber.h header */

/* Misc Helpers */
/* TEMP: only infinities, fixints and bigints for now */
inline bool kfast_zerop(TValue n) { return ttisfixint(n) && ivalue(n) == 0; }
inline bool kfast_onep(TValue n) { return ttisfixint(n) && ivalue(n) == 1; }
/* TEMP: only exact infinties */
inline TValue kneg_inf(TValue i) 
{ 
    return tv_equal(i, KEPINF)? KEMINF : KEPINF; 
}

inline bool knum_same_signp(TValue n1, TValue n2) 
{ 
    return kpositivep(n1) == kpositivep(n2); 
}

#endif
