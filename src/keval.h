/*
** keval.h
** klisp eval function
** See Copyright Notice in klisp.h
*/

#ifndef keval_h
#define keval_h

#include "kstate.h"

void keval_ofn(klisp_State *K);
/* init continuation names */
void kinit_eval_cont_names(klisp_State *K);

#endif
