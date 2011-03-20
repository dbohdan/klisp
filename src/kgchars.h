/*
** kgchars.h
** Characters features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgchars_h
#define kgchars_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 14.1.1? char? */
/* uses typep */

/* 14.1.2? char-alphabetic?, char-numeric?, char-whitespace? */
/* use ftyped_predp */

/* 14.1.3? char-upper-case?, char-lower-case? */
/* use ftyped_predp */

/* Helpers for typed predicates */
/* XXX: this should probably be in a file kchar.h but there is no real need for 
   that file yet */
bool kcharp(TValue tv);
bool kchar_alphabeticp(TValue ch);
bool kchar_numericp(TValue ch);
bool kchar_whitespacep(TValue ch);
bool kchar_upper_casep(TValue ch);
bool kchar_lower_casep(TValue ch);

/* 14.1.4? char->integer, integer->char */
void kchar_to_integer(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);
void kinteger_to_char(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* 14.1.4? char-upcase, char-downcase */
void kchar_upcase(klisp_State *K, TValue *xparams, TValue ptree, 
		  TValue denv);
void kchar_downcase(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 14.2.1? char=? */
/* uses ftyped_bpredp */

/* 14.2.2? char<?, char<=?, char>?, char>=? */
/* use ftyped_bpredp */

/* 14.2.3? char-ci=? */
/* uses ftyped_bpredp */

/* 14.2.4? char-ci<?, char-ci<=?, char-ci>?, char-ci>=? */
/* use ftyped_bpredp */

/* Helpers for typed binary predicates */
/* XXX: this should probably be in a file kchar.h but there is no real need for 
   that file yet */
bool kchar_eqp(TValue ch1, TValue ch2);
bool kchar_ltp(TValue ch1, TValue ch2);
bool kchar_lep(TValue ch1, TValue ch2);
bool kchar_gtp(TValue ch1, TValue ch2);
bool kchar_gep(TValue ch1, TValue ch2);

bool kchar_ci_eqp(TValue ch1, TValue ch2);
bool kchar_ci_ltp(TValue ch1, TValue ch2);
bool kchar_ci_lep(TValue ch1, TValue ch2);
bool kchar_ci_gtp(TValue ch1, TValue ch2);
bool kchar_ci_gep(TValue ch1, TValue ch2);

#endif
