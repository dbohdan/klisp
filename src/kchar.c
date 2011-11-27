/*
** kchar.c
** Kernel Characters
** See Copyright Notice in klisp.h
*/

#include <ctype.h>
#include <stdbool.h>

#include "kobject.h"

bool kcharp(TValue tv) { return ttischar(tv); }
bool kchar_alphabeticp(TValue ch) { return isalpha(chvalue(ch)) != 0; }
bool kchar_numericp(TValue ch)    { return isdigit(chvalue(ch)) != 0; }
bool kchar_whitespacep(TValue ch) { return isspace(chvalue(ch)) != 0; }
bool kchar_upper_casep(TValue ch) { return isupper(chvalue(ch)) != 0; }
bool kchar_lower_casep(TValue ch) { return islower(chvalue(ch)) != 0; }

/* Helpers for binary typed predicates */
bool kchar_eqp(TValue ch1, TValue ch2) { return chvalue(ch1) == chvalue(ch2); }
bool kchar_ltp(TValue ch1, TValue ch2) { return chvalue(ch1) <  chvalue(ch2); }
bool kchar_lep(TValue ch1, TValue ch2) { return chvalue(ch1) <= chvalue(ch2); }
bool kchar_gtp(TValue ch1, TValue ch2) { return chvalue(ch1) >  chvalue(ch2); }
bool kchar_gep(TValue ch1, TValue ch2) { return chvalue(ch1) >= chvalue(ch2); }

bool kchar_ci_eqp(TValue ch1, TValue ch2)
{ return tolower(chvalue(ch1)) == tolower(chvalue(ch2)); }

bool kchar_ci_ltp(TValue ch1, TValue ch2)
{ return tolower(chvalue(ch1)) <  tolower(chvalue(ch2)); }

bool kchar_ci_lep(TValue ch1, TValue ch2)
{ return tolower(chvalue(ch1)) <= tolower(chvalue(ch2)); }

bool kchar_ci_gtp(TValue ch1, TValue ch2)
{ return tolower(chvalue(ch1)) > tolower(chvalue(ch2)); }

bool kchar_ci_gep(TValue ch1, TValue ch2)
{ return tolower(chvalue(ch1)) >= tolower(chvalue(ch2)); }

