/*
** klibrary.h
** Kernel Libraries
** See Copyright Notice in klisp.h
*/

#ifndef klibrary_h
#define klibrary_h

#include "kobject.h"
#include "kstate.h"

/* GC: Assumes env & ext_list are roooted */
/* ext_list should be immutable */
TValue kmake_library(klisp_State *K, TValue env, TValue exp_list);

#define klibrary_env(p_) (tv2lib(p_)->env)
#define klibrary_exp_list(p_) (tv2lib(p_)->exp_list)

#endif
