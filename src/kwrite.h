/*
** kwrite.h
** Writer for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef kwrite_h
#define kwrite_h

#include <stdio.h>

#include "kobject.h"

/*
** Writer interface
*/
void kwrite_init();
void kwrite(TValue);
void knewline();

/* TODO: move this to the global state */
FILE *kwrite_file;

#endif

