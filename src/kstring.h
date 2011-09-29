/*
** kstring.h
** Kernel Strings
** See Copyright Notice in klisp.h
*/

/* SOURCE NOTE: the string table & hashing code is from lua */

#ifndef kstring_h
#define kstring_h

#include <stdbool.h>

#include "kobject.h"
#include "kstate.h"

/* for immutable string table */
void klispS_resize (klisp_State *K, int32_t newsize);

/* General constructor for strings */
TValue kstring_new_bs_g(klisp_State *K, bool m, const char *buf, 
			uint32_t size);

/* 
** Constructors for immutable strings
*/

/* main immutable string constructor */
/* with buffer & size */
TValue kstring_new_bs_imm(klisp_State *K, const char *buf, uint32_t size);

/* with just buffer, no embedded '\0's */
TValue kstring_new_b_imm(klisp_State *K, const char *buf);

/* 
** Constructors for mutable strings
*/

/* main mutable string constructor */
/* with just size */
TValue kstring_new_s(klisp_State *K, uint32_t size);
/* with buffer & size */
TValue kstring_new_bs(klisp_State *K, const char *buf, uint32_t size);
/* with just buffer, no embedded '\0's */
TValue kstring_new_b(klisp_State *K, const char *buf);
/* with size & fill char */
TValue kstring_new_sf(klisp_State *K, uint32_t size, char fill);

/* macros for mutable & immutable versions of the above */
#if 0
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
#endif

/* some macros to access the parts of the string */
#define kstring_buf(tv_) (tv2str(tv_)->b)
#define kstring_size(tv_) (tv2str(tv_)->size)

#define kstring_emptyp(tv_) (kstring_size(tv_) == 0)
#define kstring_mutablep(tv_) (kis_mutable(tv_))
#define kstring_immutablep(tv_) (kis_immutable(tv_))

/* both obj1 and obj2 should be strings, this compares char by char
  and doesn't differentiate immutable from mutable strings */
bool kstring_equalp(TValue obj1, TValue obj2);
bool kstringp(TValue obj);

#endif
