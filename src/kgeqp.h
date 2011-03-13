/*
** kgeqp.c
** Equivalence under mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgeqp_h
#define kgeqp_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kghelpers.h"

/* Helper (also used in equal?) */
/* TEMP: for now this is the same as tv_equal,
   later it will change with numbers and immutable objects */
inline bool eq2p(klisp_State *K, TValue obj1, TValue obj2)
{
    return (tv_equal(obj1, obj2));
}

/* 4.2.1 eq? */
/* TEMP: for now it takes only two argument */
void eqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

#endif
