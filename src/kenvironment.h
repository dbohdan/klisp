/*
** kenvironment.h
** Kernel Environments
** See Copyright Notice in klisp.h
*/

#ifndef kenvironment_h
#define kenvironment_h

#include "kobject.h"
#include "kstate.h"

/* TEMP: for now allow only a single parent */
TValue kmake_environment(klisp_State *K, TValue parent);
void kadd_binding(klisp_State *K, TValue env, TValue sym, TValue val);
TValue kget_binding(klisp_State *K, TValue env, TValue sym);

#endif
