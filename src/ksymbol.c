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
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"
/* for immutable table */
#include "kstring.h" 

/* NOTE: symbols can have source info, they should be compared with
   tv_sym_equal, NOT tv_equal */

/* TEMP: for now only interned symbols are the ones that don't 
   have source info (like those created with string->symbol) */
TValue ksymbol_new_g(klisp_State *K, const char *buf, int32_t size, 
		     TValue si, bool identifierp)
{
    /* First calculate the hash */
    uint32_t h = size; /* seed */
    size_t step = (size>>5)+1; /* if string is too long, don't hash all 
				  its chars */
    size_t size1;
    for (size1 = size; size1 >= step; size1 -= step)  /* compute hash */
	h = h ^ ((h<<5)+(h>>2)+ ((unsigned char) buf[size1-1]));

    h = ~h; /* symbol hash should be different from string hash
	       otherwise symbols and their respective immutable string
	       would always fall in the same bucket */
    /* look for it in the table only if it doesn't have source info */
    if (ttisnil(si)) {
	GCObject *o;
	for (o = K->strt.hash[lmod(h, K->strt.size)];
	     o != NULL; o = o->gch.next) {
	    String *ts = NULL;
	    if (o->gch.tt == K_TSTRING || o->gch.tt == K_TBYTEVECTOR) {
		continue; 
	    } else if (o->gch.tt == K_TSYMBOL) {
		ts = tv2str(((Symbol *) o)->str);
	    } else {
		/* only symbols, immutable bytevectors and immutable strings */
		klisp_assert(0);
	    }
	    if (ts->size == size && (memcmp(buf, ts->b, size) == 0)) {
		/* symbol may be dead */
		if (isdead(K, o)) changewhite(o);
		return gc2sym(o);
	    }
	} 
    }
    /* REFACTOR: move this to a new function */
    /* Didn't find it, alloc new immutable string and save in symbol table,
     note that the hash value remained in h */
    TValue new_str = kstring_new_bs_imm(K, buf, size);
    krooted_tvs_push(K, new_str);
    Symbol *new_sym = klispM_new(K, Symbol);
    TValue ret_tv = gc2sym(new_sym);
    krooted_tvs_pop(K); 
    
    if (ttisnil(si)) {
	/* header + gc_fields */
	/* can't use klispC_link, because strings use the next pointer
	   differently */
	new_sym->gct = klispC_white(K);
	new_sym->tt = K_TSYMBOL;
	new_sym->kflags = identifierp? K_FLAG_EXT_REP : 0;
	new_sym->si = NULL;

	/* symbol specific fields */
	new_sym->str = new_str;
	new_sym->hash = h;

	/* add to the string/symbol table (and link it) */
	stringtable *tb;
	tb = &K->strt;
	h = lmod(h, tb->size);
	new_sym->next = tb->hash[h];  /* chain new entry */
	tb->hash[h] = (GCObject *)(new_sym);
	tb->nuse++;
	if (tb->nuse > ((uint32_t) tb->size) && tb->size <= INT32_MAX / 2) {
	    krooted_tvs_push(K, ret_tv); /* save in case of gc */
	    klispS_resize(K, tb->size*2);  /* too crowded */
	    krooted_tvs_pop(K);
	}
    } else { /* non nil source info */
	/* link it with regular objects and save source info */
	/* header + gc_fields */
	klispC_link(K, (GCObject *) new_sym, K_TSYMBOL,
	    identifierp? K_FLAG_EXT_REP : 0);
	
	/* symbol specific fields */
	new_sym->str = new_str;
	new_sym->hash = h;

	krooted_tvs_push(K, ret_tv); /* not needed, but just in case */
	kset_source_info(K, ret_tv, si);
	krooted_tvs_pop(K); 
    }
    return ret_tv;
}

/* for indentifiers */
TValue ksymbol_new_i(klisp_State *K, const char *buf, int32_t size, TValue si)
{
    return ksymbol_new_g(K, buf, size, si, true);
}

/* for indentifiers with no size */
TValue ksymbol_new(klisp_State *K, const char *buf, TValue si)
{
    int32_t size = (int32_t) strlen(buf);
    return ksymbol_new_g(K, buf, size, si, true);
}

/* for string->symbol */
/* GC: assumes str is rooted */
TValue ksymbol_new_check_i(klisp_State *K, TValue str, TValue si)
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
	    if (!ktok_is_subsequent(*buf))
		identifierp = false;
	    else
		buf++;
	}
    }
    /* recover size & buf*/
    size = kstring_size(str);
    buf = kstring_buf(str);

    TValue new_sym = ksymbol_new_g(K, buf, size, si, identifierp);
    return new_sym;
}

bool ksymbolp(TValue obj) { return ttissymbol(obj); }
