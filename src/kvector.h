/*
** kvector.h
** Kernel Vectors (heterogenous arrays)
** See Copyright Notice in klisp.h
*/

#ifndef kvector_h
#define kvector_h

#include "kobject.h"
#include "kstate.h"

/* constructors */

TValue kvector_new_sf(klisp_State *K, uint32_t length, TValue fill);
TValue kvector_new_bs_g(klisp_State *K, bool m,
                        const TValue *buf, uint32_t length);

/* predicates */

bool kvectorp(TValue obj);
bool kimmutable_vectorp(TValue obj);
bool kmutable_vectorp(TValue obj);

/* some macros to access the parts of vectors */

#define kvector_array(tv_) (tv2vector(tv_)->array)
#define kvector_length(tv_) (tv2vector(tv_)->sizearray)

#define kvector_emptyp(tv_) (kvector_length(tv_) == 0)
#define kvector_mutablep(tv_) (kis_mutable(tv_))
#define kvector_immutablep(tv_) (kis_immutable(tv_))

#endif
