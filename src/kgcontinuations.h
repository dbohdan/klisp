/*
** kgcontinuations.h
** Continuations features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcontinuations_h
#define kgcontinuations_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 7.1.1 continuation? */
/* uses typep */

/* 7.2.2 call/cc */
void call_cc(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 7.2.3 extend-continuation */
/* TODO */

/* 7.2.4 guard-continuation */
/* TODO */

/* 7.2.5 continuation->applicative */
/* TODO */

/* 7.2.6 root-continuation */
/* TODO */

/* 7.2.7 error-continuation */
/* TODO */

/* 7.3.1 apply-continuation */
/* TODO */

/* 7.3.2 $let/cc */
/* TODO */

/* 7.3.3 guard-dynamic-extent */
/* TODO */

/* 7.3.4 exit */    
/* TODO */

#endif
