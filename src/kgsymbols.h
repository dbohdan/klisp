/*
** kgsymbols.h
** Symbol features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgsymbols_h
#define kgsymbols_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.4.1 symbol? */
/* uses typep */

/* ?.?.1? symbol->string */
void symbol_to_string(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* ?.?.2? string->symbol */
/* TEMP: for now this can create symbols with no external representation
   this includes all symbols with non identifiers characters.
*/
/* NOTE:
   Symbols with uppercase alphabetic characters will write as lowercase and
   so, when read again will not compare as either eq? or equal?. This is ok
   because the report only says that read objects when written and read 
   again must be equal? which happens here 
*/
void string_to_symbol(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* init ground */
void kinit_symbols_ground_env(klisp_State *K);

#endif
