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

#include "kstate.h"

/* init ground */
void kinit_ffi_ground_env(klisp_State *K);
/* init continuation names */
void kinit_ffi_cont_names(klisp_State *K);

#endif
