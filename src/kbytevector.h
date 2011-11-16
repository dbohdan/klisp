/*
** kbytevector.h
** Kernel Byte Vectors
** See Copyright Notice in klisp.h
*/

#ifndef kbytevector_h
#define kbytevector_h

#include "kobject.h"
#include "kstate.h"

/* TODO change bytevector constructors to string like constructors */
/* TODO change names to lua-like (e.g. klispB_new, etc) */

/* Constructors for bytevectors */
TValue kbytevector_new_g(klisp_State *K, bool m, uint32_t size);
TValue kbytevector_new_imm(klisp_State *K, uint32_t size);
TValue kbytevector_new(klisp_State *K, uint32_t size);

/* both obj1 and obj2 should be bytevectors, this compares byte by byte
  and doesn't differentiate immutable from mutable bytevectors */
bool kbytevector_equalp(TValue obj1, TValue obj2);
bool kbytevector(TValue obj);

/* some macros to access the parts of the bytevectors */
#define kbytevector_buf(tv_) (tv2bytevector(tv_)->b)
#define kbytevector_size(tv_) (tv2bytevector(tv_)->size)

#define kbytevector_emptyp(tv_) (kbytevector_size(tv_) == 0)
#define kbytevector_mutablep(tv_) (kis_mutable(tv_))
#define kbytevector_immutablep(tv_) (kis_immutable(tv_))

#endif
