/*
** kchar.h
** Kernel Characters
** See Copyright Notice in klisp.h
*/

#ifndef kchar_h
#define kchar_h

#include <stdbool.h>

#include "kobject.h"
#include "kstate.h"

bool kcharp(TValue tv);
bool kchar_alphabeticp(TValue ch);
bool kchar_numericp(TValue ch);
bool kchar_whitespacep(TValue ch);
bool kchar_upper_casep(TValue ch);
bool kchar_lower_casep(TValue ch);
bool kchar_title_casep(TValue ch);
/* Helpers for binary typed predicates */
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
