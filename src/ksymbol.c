/*
** ksymbol.c
** Kernel Symbols
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "ksymbol.h"
#include "kobject.h"
#include "kpair.h"
#include "kstate.h"
#include "kmem.h"

TValue ksymbol_new(klisp_State *K, const char *buf)
{
    /* TODO: replace symbol list with hashtable */
    /* First look for it in the symbol table */
    TValue tbl = K->symbol_table;
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
    Symbol *new_sym = klispM_malloc(K, sizeof(Symbol) + size + 1);

    new_sym->next = NULL;
    new_sym->gct = 0;
    new_sym->tt = K_TSYMBOL;
    new_sym->mark = KFALSE;
    new_sym->size = size;
    memcpy(new_sym->b, buf, size);
    new_sym->b[size] = '\0';

    TValue new_symv = gc2sym(new_sym);
    /* XXX: new_symv unrooted */
    K->symbol_table = kcons(K, new_symv, K->symbol_table);
    return new_symv;
}
