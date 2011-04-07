/*
** kgcombiners.h
** Combiners features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcombiners_h
#define kgcombiners_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.10.1 operative? */
/* uses typep */

/* 4.10.2 applicative? */
/* uses typep */

/* 4.10.3 $vau */
/* 5.3.1 $vau */
void Svau(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 4.10.4 wrap */
void wrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 4.10.5 unwrap */
void unwrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.3.1 $vau */
/* DONE: above, together with 4.10.4 */

/* 5.3.2 $lambda */
void Slambda(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.5.1 apply */
void apply(klisp_State *K, TValue *xparams, TValue ptree, 
	   TValue denv);

/* 5.9.1 map */
void map(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.2.1 combiner? */
/* uses ftypedp */

/* Helper for combiner? */
bool kcombinerp(TValue obj);

#endif
