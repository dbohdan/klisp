/*
** ktoken.h
** Tokenizer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef ktoken_h
#define ktoken_h

#include "kobject.h"

/*
** Tokenizer interface
*/
void ktok_init();
TValue ktok_read_token();

/* TODO: move this to the global state */
FILE *ktok_file;

#endif
