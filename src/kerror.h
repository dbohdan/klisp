/*
** kerror.h
** Simple error notification and handling (TEMP)
** See Copyright Notice in klisp.h
*/


#ifndef kerror_h
#define kerror_h

#include <stdbool.h>

#include "klisp.h"
#include "kstate.h"

void klispE_throw(klisp_State *K, char *msg, bool can_cont);

#endif
