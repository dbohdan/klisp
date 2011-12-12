/*
** kmodule.h
** Kernel Modules
** See Copyright Notice in klisp.h
*/

#ifndef kmodule_h
#define kmodule_h

#include "kobject.h"
#include "kstate.h"

/* GC: Assumes env & ext_list are roooted */
/* ext_list should be immutable */
TValue kmake_module(klisp_State *K, TValue env, TValue exp_list);

#define kmodule_env(p_) (tv2mod(p_)->env)
#define kmodule_exp_list(p_) (tv2mod(p_)->exp_list)

#endif
