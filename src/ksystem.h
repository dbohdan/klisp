/*
** ksystem.h
** Platform dependent functionality.
** See Copyright Notice in klisp.h
*/

#ifndef ksystem_h
#define ksystem_h

#include "kobject.h"

TValue ksystem_current_jiffy(klisp_State *K);
TValue ksystem_jiffies_per_second(klisp_State *K);

#endif

