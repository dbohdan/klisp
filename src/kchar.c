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
