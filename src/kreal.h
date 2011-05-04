/*
** kreal.c
** Kernel Reals (doubles)
** See Copyright Notice in klisp.h
*/

#ifndef kreal_h
#define kreal_h

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "kobject.h"
#include "kstate.h"
#include "kinteger.h"
#include "imrat.h"

TValue kexact_to_inexact(klisp_State *K, TValue n);


/*
** read/write interface 
*/
int32_t kdouble_print_size(TValue tv_double);
void  kdouble_print_string(klisp_State *K, TValue tv_double,
			   char *buf, int32_t limit);

#endif
