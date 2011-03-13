/*
** kgequalp.h
** Equivalence up to mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgequalp_h
#define kgequalp_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kghelpers.h"

/* 4.3.1 equal? */
/* TEMP: for now it takes only two argument */
void equalp(klisp_State *K, TValue *xparas, TValue ptree, TValue denv);

/* Helper (may be used in assoc and member) */
/* compare two objects and check to see if they are "equal?". */
bool equal2p(klisp_State *K, TValue obj1, TValue obj2);

#endif
