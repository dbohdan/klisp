/*
** kgbooleans.h
** Boolean features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgbooleans_h
#define kgbooleans_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.1.1 boolean? */
/* uses typep */

/* 6.1.1 not? */
void notp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.1.2 and? */
void andp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.1.3 or? */
void orp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.1.4 $and? */
/* TODO */

/* 6.1.5 $or? */
/* TODO */

/* Helper */
bool kbooleanp(TValue obj);

#endif
