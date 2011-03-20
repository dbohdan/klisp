/*
** kgnumbers.c
** Numbers features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgnumbers.h"

/* 15.5.1? number?, finite?, integer? */
/* use ftypep & ftypep_predp */

bool knumberp(TValue obj) { return ttype(obj) <= K_LAST_NUMBER_TYPE; }
/* obj is known to be a number */
bool kfinitep(TValue obj) { return (!ttiseinf(obj) && !ttisiinf(obj)); }
/* TEMP: for now only fixint, should also include bigints and 
   inexact integers */
bool kintegerp(TValue obj) { return ttisfixint(obj); }


