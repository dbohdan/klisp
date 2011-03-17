/*
** kport.h
** Kernel Ports
** See Copyright Notice in klisp.h
*/

#ifndef kport_h
#define kport_h

#include <stdio.h>

#include "kobject.h"
#include "kstate.h"

TValue kmake_port(klisp_State *K, TValue filename, bool writep, TValue name, 
			  TValue si);

/* this is for creating ports for stdin/stdout/stderr */
TValue kmake_std_port(klisp_State *K, TValue filename, bool writep, 
		      TValue name, TValue si, FILE *file);

/* This closes the underlying FILE * (unless it is a std port) and also
   set the closed flag to true */
void kclose_port(klisp_State *K, TValue port);

#endif
