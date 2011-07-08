/*
** kblob.h
** Kernel Blobs (byte vectors)
** See Copyright Notice in klisp.h
*/

#ifndef kblob_h
#define kblob_h

#include "kobject.h"
#include "kstate.h"

/* General constructor for blobs */
TValue kblob_new_g(klisp_State *K, bool m, uint32_t size);
TValue kblob_new_imm(klisp_State *K, bool m, uint32_t size);
TValue kblob_new(klisp_State *K, bool m, uint32_t size);

/* both obj1 and obj2 should be blobs, this compares byte by byte
  and doesn't differentiate immutable from mutable blobs */
bool kblob_equalp(TValue obj1, TValue obj2);
bool kblob(TValue obj);

/* some macros to access the parts of the blobs */
#define kblob_buf(tv_) (tv2blob(tv_)->b)
#define kblob_size(tv_) (tv2blob(tv_)->size)

#define kblob_mutablep(tv_) (kis_mutable(tv_))
#define kblob_immutablep(tv_) (kis_immutable(tv_))

#endif
