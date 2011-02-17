/*
** ksymbol.h
** Kernel Symbols
** See Copyright Notice in klisp.h
*/

#ifndef ksymbol_h
#define ksymbol_h

#include "kobject.h"

/* TODO: replace the list with a hashtable */
/* TODO: move to global state */
TValue ksymbol_table;

/* XXX: for now all symbols are interned */
TValue ksymbol_new(const char *);

#endif
