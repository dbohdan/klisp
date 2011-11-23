/*
** klisp.h
** klisp - An interpreter for the Kernel Programming Language.
** See Copyright Notice at the end of this file
*/

#ifndef klisp_h
#define klisp_h

#include <stdlib.h>

/* NOTE: this inclusion is reversed in lua */
#include "kobject.h"

/*
** SOURCE NOTE: This is mostly from Lua.
*/

#define KLISP_VERSION	"klisp 0.2"
#define KLISP_RELEASE	"klisp 0.2"
#define KLISP_VERSION_NUM	02
#define KLISP_COPYRIGHT	"Copyright (C) 2011 Andres Navarro, Oto Havle"
#define KLISP_AUTHORS 	"Andres Navarro, Oto Havle"

typedef struct klisp_State klisp_State;

/*
** prototype for memory-allocation functions
*/
typedef void * (*klisp_Alloc) 
    (void *ud, void *ptr, size_t osize, size_t nsize);

/*
** prototype for callable c functions from the interpreter main loop:
**
** TEMP: for now it is defined in kobject.h
*/
/* typedef void (*klisp_Ifunc) (TValue *ud, TValue val); */

/*
** state manipulation
*/
klisp_State *klisp_newstate (klisp_Alloc f, void *ud);
void klisp_close (klisp_State *K);

/******************************************************************************
* Copyright (C) 2011 Andres Navarro, Oto Havle.
* Lua parts: Copyright (C) 1994-2010 Lua.org, PUC-Rio.
* IMath Parts: Copyright (C) 2002-2007 Michael J. Fromberger.
* srfi-78: Copyright (C) 2005-2006 Sebastian Egner.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
******************************************************************************/

#endif
