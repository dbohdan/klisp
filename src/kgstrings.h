/*
** kgstrings.h
** Strings features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgstrings_h
#define kgstrings_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 13.1.1? string? */
/* uses typep */

/* 13.1.2? make-string */
void make_string(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.1.3? string-length */
void string_length(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv);

/* 13.1.4? string-ref */
void string_ref (klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.1.5? string-set! */
void string_setS (klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.2.1? string */
void string(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.2.2? string=?, string-ci=? */
/* TODO */

/* 13.2.3? string<?, string<=?, string>?, string>=? */
/* TODO */

/* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
/* TODO */

/* 13.2.5? substring */
void substring(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.2.6? string-append */
/* TODO */

/* 13.2.7? string->list, list->string */
/* TODO */

/* 13.2.8? string-copy */
void string_copy(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.2.9? string-fill! */
void string_fillS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 13.3.1? symbol->string */
/* TEMP: for now all strings are mutable, this returns a new object
   each time */
/* TODO */

/* 13.3.2? symbol->string */
/* TEMP: for now this can create symbols with no external representation
   this includes all symbols with non identifiers characters.
*/
/* NOTE:
   Symbols with uppercase alphabetic characters will write as lowercase and
   so, when read again will not compare as either eq? or equal?. This is ok
   because the report only says that read objects when written and read 
   again must be equal? which happens here 
*/
/* TODO */

#endif
