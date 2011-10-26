/*
** kgffi.h
** Foreign function interface
** See Copyright Notice in klisp.h
*/

#ifndef kgffi_h
#define kgffi_h

#if (KUSE_LIBFFI != 1)
#    error "Compiling FFI code, but KUSE_LIBFFI != 1."
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

void ffi_load_library(klisp_State *K, TValue *xparams,
                      TValue ptree, TValue denv);

/* init ground */
void kinit_ffi_ground_env(klisp_State *K);

#endif
