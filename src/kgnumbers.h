/*
** kgnumbers.h
** Numbers features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgnumbers_h
#define kgnumbers_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 15.5.1 number?, finite?, integer? */
/* use ftypep & ftypep_predp */

/* Helpers for typed predicates */
/* XXX: this should probably be in a file knumber.h but there is no real need for 
   that file yet */
bool knumberp(TValue obj);
bool kfinitep(TValue obj);
bool kintegerp(TValue obj);

#endif
