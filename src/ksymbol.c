/*
** ksymbol.c
** Kernel Symbols
** See Copyright Notice in klisp.h
*/

/* XXX: for malloc */
#include <stdlib.h>
/* TODO: use a generalized alloc function */

#include <string.h>

#include "ksymbol.h"
#include "kobject.h"
#include "kpair.h"

/* TODO: replace the list with a hashtable */
/* TODO: move to global state */
TValue ksymbol_table = KNIL_;

/* TODO: Out of memory errors */
TValue ksymbol_new(const char *buf)
{
    /* First look for it in the symbol table */
    TValue tbl = ksymbol_table;
    while (!ttisnil(tbl)) {
	TValue first = kcar(tbl);
	/* NOTE: there are no embedded '\0's in symbols */
	if (strcmp(buf, tv2sym(first)->b) == 0)
	    return first;
	else
	    tbl = kcdr(tbl);
    }

    /* Didn't find it, alloc new and save in symbol table */
    /* NOTE: there are no embedded '\0's in symbols */
    int32_t size = strlen(buf);
    Symbol *new_sym = malloc(sizeof(Symbol) + size + 1);

    new_sym->next = NULL;
    new_sym->gct = 0;
    new_sym->tt = K_TSYMBOL;
    new_sym->size = size;
    memcpy(new_sym->b, buf, size);
    new_sym->b[size] = '\0';

    TValue new_symv = gc2sym(new_sym);
    tbl = kcons(new_symv, ksymbol_table);
    ksymbol_table = tbl;
    return new_symv;
}
