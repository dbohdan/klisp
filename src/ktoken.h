/*
** ktoken.h
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef ktoken_h
#define ktoken_h

#include "kobject.h"
#include "kstate.h"

#include <stdio.h>

/*
** Tokenizer interface
*/
void ktok_init(klisp_State *K);
TValue ktok_read_token(klisp_State *K);
void ktok_reset_source_info(klisp_State *K);
TValue ktok_get_source_info(klisp_State *K);

/* This is needed here to allow cleanup of shared dict from tokenizer */
void clear_shared_dict(klisp_State *K);


#endif
