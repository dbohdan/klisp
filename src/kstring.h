/*
** kstring.h
** Kernel Strings
** See Copyright Notice in klisp.h
*/

#ifndef kstring_h
#define kstring_h

#include <stdbool.h>

#include "kobject.h"
#include "kstate.h"

/* used for initialization */
TValue kstring_new_empty(klisp_State *K);
/* general string constructor, buf remains uninit
 (except for an extra trailing zero used for printing */
TValue kstring_new_s_g(klisp_State *K, bool m, uint32_t size);
/* with buffer & size */
TValue kstring_new_bs_g(klisp_State *K, bool m, const char *buf, uint32_t size);
/* with buffer but no size, no embedded '\0's */
TValue kstring_new_b_g(klisp_State *K, bool m, const char *buf);
/* with size & fill char */
TValue kstring_new_sf_g(klisp_State *K, bool m, uint32_t size, char fill);

/* macros for mutable & immutable versions of the above */
#define kstring_new_s(K_, size_)		\
    kstring_new_s_g(K_, true, size_)
#define kstring_new_bs(K_, buf_, size_)		\
    kstring_new_bs_g(K_, true, buf_, size_)
#define kstring_new_b(K_, buf_)			\
    kstring_new_b_g(K_, true, buf_)
#define kstring_new_sf(K_, size_, fill_)	\
    kstring_new_sf_g(K_, true, size_, fill_)

#define kstring_new_s_imm(K_, size_)		\
    kstring_new_s_g(K_, false, size_)
#define kstring_new_bs_imm(K_, buf_, size_)	\
    kstring_new_bs_g(K_, false, buf_, size_)
#define kstring_new_b_imm(K_, buf_)		\
    kstring_new_b_g(K_, false, buf_)
#define kstring_new_sf_imm(K_, size_, fill_)	\
    kstring_new_sf_g(K_, false, size_, fill_)

/* some macros to access the parts of the string */
#define kstring_buf(tv_) (tv2str(tv_)->b)
#define kstring_size(tv_) (tv2str(tv_)->size)

#define kstring_emptyp(tv_) (kstring_size(tv_) == 0)
#define kstring_mutablep(tv_) (kis_mutable(tv_))
#define kstring_immutablep(tv_) (kis_immutable(tv_))

/* both obj1 and obj2 should be strings, this compares char by char
  but differentiates immutable from mutable strings */
bool kstring_equalp(TValue obj1, TValue obj2);

#endif
