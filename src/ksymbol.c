/*
** ksymbol.c
** Kernel Symbols
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "ksymbol.h"
#include "kobject.h"
/* for identifier checking */
#include "ktoken.h"
#include "kpair.h"
#include "kstate.h"
#include "kmem.h"

TValue ksymbol_new_g(klisp_State *K, const char *buf, int32_t size, 
		     bool identifierp)
{
    /* TODO: replace symbol list with hashtable */
    /* First look for it in the symbol table */
    TValue tbl = K->symbol_table;

    while (!ttisnil(tbl)) {
	TValue first = kcar(tbl);
	/* NOTE: there are no embedded '\0's in identifiers but 
	 they could be in other symbols */
	if (size == ksymbol_size(first) && 
	    memcmp(buf, ksymbol_buf(first), size) == 0) {
	    return first;
	} else
	    tbl = kcdr(tbl);
    }

    /* Didn't find it, alloc new string and save in symbol table */
    /* NOTE: there are no embedded '\0's in symbols */
    /* GC: root new_str */
    TValue new_str = kstring_new(K, buf, size); /* this copies the buf */
    Symbol *new_sym = klispM_new(K, Symbol);
    
    /* header + gc_fields */
    new_sym->next = K->root_gc;
    K->root_gc = (GCObject *)new_sym;
    new_sym->gct = 0;
    new_sym->tt = K_TSYMBOL;
    new_sym->flags = identifierp? K_FLAG_EXT_REP : 0; 

    /* symbol specific fields */
    new_sym->mark = KFALSE;
    new_sym->str = new_str;

    TValue new_symv = gc2sym(new_sym);
    /* GC: root new_symb */
    K->symbol_table = kcons(K, new_symv, K->symbol_table);
    return new_symv;
}

/* for indentifiers */
TValue ksymbol_new_i(klisp_State *K, const char *buf, int32_t size)
{
    return ksymbol_new_g(K, buf, size, true);
}

/* for indentifiers with no size */
TValue ksymbol_new(klisp_State *K, const char *buf)
{
    int32_t size = (int32_t) strlen(buf);
    return ksymbol_new_g(K, buf, size, true);
}

/* for string->symbol */
TValue ksymbol_new_check_i(klisp_State *K, TValue str)
{
    int32_t size = kstring_size(str);
    char *buf = kstring_buf(str);
    bool identifierp;
    
    /* this is necessary because the empty symbol isn't an identifier */
    /* MAYBE it should throw an error if the string is empty */
    /* XXX: The exact syntax for identifiers isn't there in the report
       yet, here we use something like scheme, and the same as in ktoken.h
       (details, leading numbers '.', '+' and '-' are a no go, but '+' and
       '-' are an exception.
    */
    identifierp = (size > 0);
    if (identifierp) {
	char first = *buf;
	buf++;
	size--;
	if (first == '+' || first == '-')
	    identifierp = (size == 0);
	else if (first == '.' || ktok_is_numeric(first))
	    identifierp = false;
	else 
	    identifierp = ktok_is_subsequent(first);

	while(identifierp && size--) {
	    if (ktok_is_subsequent(*buf))
		identifierp = false;
	    else
		buf++;
	}
    }
    /* recover size & buf*/
    size = kstring_size(str);
    buf = kstring_buf(str);
    return ksymbol_new_g(K, buf, size, identifierp);
}
