/*
** kencapsulation.h
** Kernel Encapsulation Types
** See Copyright Notice in klisp.h
*/

#ifndef kencapsulation_h
#define kencapsulation_h

#include "kobject.h"
#include "kstate.h"

TValue kmake_encapsulation(klisp_State *K, TValue name, TValue si,
			   TValue key, TValue val);
TValue kmake_encapsulation_key(klisp_State *K);
inline bool kis_encapsulation_type(TValue enc, TValue key);

#define kget_enc_val(e_)(tv2enc(e_)->value)
#define kget_enc_key(e_)(tv2enc(e_)->key)

inline bool kis_encapsulation_type(TValue enc, TValue key)
{
    return ttisencapsulation(enc) && tv_equal(kget_enc_key(enc), key);
}


#endif
