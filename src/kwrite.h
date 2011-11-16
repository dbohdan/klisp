/*
** kwrite.h
** Writer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef kwrite_h
#define kwrite_h

#include "kobject.h"
#include "kstate.h"

/*
** Writer interface
*/
void kwrite_display_to_port(klisp_State *K, TValue port, TValue obj, 
			    bool displayp);
void kwrite_newline_to_port(klisp_State *K, TValue port);
void kwrite_char_to_port(klisp_State *K, TValue port, TValue ch);
void kwrite_u8_to_port(klisp_State *K, TValue port, TValue u8);

#endif

