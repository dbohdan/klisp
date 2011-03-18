/*
** kgchars.c
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
/* TODO */

/* 14.2.1? char=? */
/* TODO */

/* 14.2.2? char<?, char<=?, char>?, char>=? */
/* TODO */

/* 14.2.3? char-ci=? */
/* TODO */

/* 14.2.4? char-ci<?, char-ci<=?, char-ci>?, char-ci>=? */
/* TODO */

#endif
