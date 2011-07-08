/*
** kgblobs.h
** Blobs features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgblobs_h
#define kgblobs_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* ??.1.1? blob? */
/* uses typep */

/* ??.1.2? make-blob */
void make_blob(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* ??.1.3? blob-length */
void blob_length(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv);

/* ??.1.4? blob-ref */
void blob_ref(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* ??.1.5? blob-set! */
void blob_setS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* init ground */
void kinit_blobs_ground_env(klisp_State *K);

#endif
