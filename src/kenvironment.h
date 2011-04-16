/*
** kenvironment.h
** Kernel Environments
** See Copyright Notice in klisp.h
*/

#ifndef kenvironment_h
#define kenvironment_h

#include "kobject.h"
#include "kstate.h"

TValue kmake_environment(klisp_State *K, TValue parents);
#define kmake_empty_environment(kst_) (kmake_environment(kst_, KNIL))
void kadd_binding(klisp_State *K, TValue env, TValue sym, TValue val);
TValue kget_binding(klisp_State *K, TValue env, TValue sym);
bool kbinds(klisp_State *K, TValue env, TValue sym);
/* keyed dynamic vars */
TValue kmake_keyed_static_env(klisp_State *K, TValue parent, TValue key, 
			      TValue val);
TValue kget_keyed_static_var(klisp_State *K, TValue env, TValue key);

#endif
