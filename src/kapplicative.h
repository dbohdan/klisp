/*
** kapplicative.h
** Kernel Applicatives
** See Copyright Notice in klisp.h
*/

#ifndef kapplicative_h
#define kapplicative_h

#include "kobject.h"
#include "kstate.h"

TValue kwrap(klisp_State *K, TValue underlying);
TValue kmake_applicative(klisp_State *K, TValue name, TValue si, 
			 TValue underlying);
#define kunwrap(app_) (tv2app(app_)->underlying)
#endif
