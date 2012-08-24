/*
** kbytevector.h
** Kernel Byte Vectors
** See Copyright Notice in klisp.h
*/

#ifndef kbytevector_h
#define kbytevector_h

#include "kobject.h"
#include "kstate.h"

/* TODO change names to be lua-like (e.g. klispBB_new, etc) */

/* General constructor for bytevectors */
TValue kbytevector_new_bs_g(klisp_State *K, bool m, const uint8_t *buf, 
                            uint32_t size);

/* 
** Constructors for immutable bytevectors
*/

/* main immutable bytevector constructor */
/* with buffer & size */
TValue kbytevector_new_bs_imm(klisp_State *K, const uint8_t *buf, uint32_t size);

/* 
** Constructors for mutable bytevectors
*/

/* main mutable bytevector constructor */
/* with just size */
TValue kbytevector_new_s(klisp_State *K, uint32_t size);
/* with buffer & size */
TValue kbytevector_new_bs(klisp_State *K, const uint8_t *buf, uint32_t size);
/* with size & fill byte */
TValue kbytevector_new_sf(klisp_State *K, uint32_t size, uint8_t fill);

/* both obj1 and obj2 should be bytevectors, this compares byte by byte
   and doesn't differentiate immutable from mutable bytevectors */
bool kbytevector_equalp(klisp_State *K, TValue obj1, TValue obj2);
bool kbytevectorp(TValue obj);
bool kimmutable_bytevectorp(TValue obj);
bool kmutable_bytevectorp(TValue obj);

/* some macros to access the parts of the bytevectors */
/* LOCK: these are immutable, so they don't need locking */
#define kbytevector_buf(tv_) (tv2bytevector(tv_)->b)
#define kbytevector_size(tv_) (tv2bytevector(tv_)->size)

#define kbytevector_emptyp(tv_) (kbytevector_size(tv_) == 0)
#define kbytevector_mutablep(tv_) (kis_mutable(tv_))
#define kbytevector_immutablep(tv_) (kis_immutable(tv_))

#endif
