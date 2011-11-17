/*
** kgbytevectors.h
** Bytevectors features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgbytevectors_h
#define kgbytevectors_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* ??.1.1? bytevector? */
/* uses typep */

/* ??.1.2? make-bytevector */
void make_bytevector(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv);

/* ??.1.3? bytevector-length */
void bytevector_length(klisp_State *K, TValue *xparams, TValue ptree, 
		       TValue denv);

/* ??.1.4? bytevector-u8-ref */
void bytevector_u8_ref(klisp_State *K, TValue *xparams, TValue ptree, 
		       TValue denv);

/* ??.1.5? bytevector-u8-set! */
void bytevector_u8_setS(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv);

/* ??.2.?? bytevector-copy */
void bytevector_copy(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv);

/* ??.2.?? bytevector-copy! */
void bytevector_copyS(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* ??.2.?? bytevector-copy-partial */
void bytevector_copy_partial(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* ??.2.?? bytevector-copy-partial! */
void bytevector_copy_partialS(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* ??.2.?? bytevector->immutable-bytevector */
void bytevector_to_immutable_bytevector(klisp_State *K, TValue *xparams, 
					TValue ptree, TValue denv);

/* init ground */
void kinit_bytevectors_ground_env(klisp_State *K);

#endif
