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

#endif
