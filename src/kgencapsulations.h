/*
** kgencapsulations.h
** Encapsulations features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgencapsulations_h
#define kgencapsulations_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 8.1.1 make-encapsulation-type */
void make_encapsulation_type(klisp_State *K, TValue *xparams, TValue ptree,
			     TValue denv);

#endif
