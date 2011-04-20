/*
** kenvironment.h
** Kernel Environments
** See Copyright Notice in klisp.h
*/

#ifndef kenvironment_h
#define kenvironment_h

#include "kobject.h"
#include "kstate.h"

/* GC: Assumes parents is rooted */
TValue kmake_environment(klisp_State *K, TValue parents);
#define kmake_empty_environment(kst_) (kmake_environment(kst_, KNIL))
void kadd_binding(klisp_State *K, TValue env, TValue sym, TValue val);
TValue kget_binding(klisp_State *K, TValue env, TValue sym);
bool kbinds(klisp_State *K, TValue env, TValue sym);
/* keyed dynamic vars */
/* GC: Assumes parents, key & val are rooted */
TValue kmake_keyed_static_env(klisp_State *K, TValue parent, TValue key, 
			      TValue val);
TValue kget_keyed_static_var(klisp_State *K, TValue env, TValue key);

/* environments with hashtable bindings */
/* TEMP: for now only for ground environment
   TODO: Should profile too see when it makes sense & should add code 
   to all operatives creating environments to see when it's appropiate 
   or should add code to add binding to at certain point move over to 
   hashtable */
TValue kmake_table_environment(klisp_State *K, TValue parents);

#endif
