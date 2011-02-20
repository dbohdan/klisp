/*
** kread.h
** Reader for the Kernel Programming Language
** See Copyright Notice in klisp.h
*/

#ifndef kread_h
#define kread_h

#include "kobject.h"

/*
** Reader interface
*/
void kread_init();
TValue kread();

/* TODO: move this to the global state */
FILE *kread_file;
char *kread_filename;

#endif

