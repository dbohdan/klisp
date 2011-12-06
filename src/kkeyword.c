/*
** kkeywrod.c
** Kernel Keywords
** See Copyright Notice in klisp.h
*/

#include <string.h>

#include "kkeyword.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"
/* for immutable table */
#include "kstring.h" 

/* No case folding is performed by these constructors */
TValue kkeyword_new_bs(klisp_State *K, const char *buf, int32_t size)
{
    /* First calculate the hash */
    uint32_t h = size; /* seed */
    size_t step = (size>>5)+1; /* if string is too long, don't hash all 
				  its chars */
    size_t size1;
    for (size1 = size; size1 >= step; size1 -= step)  /* compute hash */
	h = h ^ ((h<<5)+(h>>2)+ ((unsigned char) buf[size1-1]));

    h ^= (uint32_t) 0x55555555; 
             /* keyword hash should be different from string & symbol hash
		otherwise keywords and their respective immutable string
		would always fall in the same bucket */
    /* look for it in the table */
    for (GCObject *o = K->strt.hash[lmod(h, K->strt.size)]; o != NULL; 
	 o = o->gch.next) {
	klisp_assert(o->gch.tt == K_TKEYWORD || o->gch.tt == K_TSYMBOL || 
		     o->gch.tt == K_TSTRING || o->gch.tt == K_TBYTEVECTOR);
		     
	if (o->gch.tt != K_TKEYWORD) continue;

	String *ts = tv2str(((Keyword *) o)->str);
	if (ts->size == size && (memcmp(buf, ts->b, size) == 0)) {
	    /* keyword and/or string may be dead */
	    if (isdead(K, o)) changewhite(o);
	    if (isdead(K, (Object *) ts)) changewhite((Object *) ts);
	    return gc2keyw(o);
	}
    } 
    /* REFACTOR: move this to a new function */
    /* Didn't find it, alloc new immutable string and save in keyword table,
     note that the hash value remained in h */
    TValue new_str = kstring_new_bs_imm(K, buf, size);
    krooted_tvs_push(K, new_str);
    Keyword *new_keyw = klispM_new(K, Keyword);
    TValue ret_tv = gc2keyw(new_keyw);
    krooted_tvs_pop(K); 
    
    /* header + gc_fields */
    /* can't use klispC_link, because strings use the next pointer
       differently */
    new_keyw->gct = klispC_white(K);
    new_keyw->tt = K_TKEYWORD;
    new_keyw->kflags = 0;
    new_keyw->si = NULL;

    /* keyword specific fields */
    new_keyw->str = new_str;
    new_keyw->hash = h;

    /* add to the string/keyword table (and link it) */
    stringtable *tb;
    tb = &K->strt;
    h = lmod(h, tb->size);
    new_keyw->next = tb->hash[h];  /* chain new entry */
    tb->hash[h] = (GCObject *)(new_keyw);
    tb->nuse++;
    if (tb->nuse > ((uint32_t) tb->size) && tb->size <= INT32_MAX / 2) {
	krooted_tvs_push(K, ret_tv); /* save in case of gc */
	klispS_resize(K, tb->size*2);  /* too crowded */
	krooted_tvs_pop(K);
    }
    return ret_tv;
}

/* for c strings with unknown size */
TValue kkeyword_new_b(klisp_State *K, const char *buf)
{
    int32_t size = (int32_t) strlen(buf);
    return kkeyword_new_bs(K, buf, size);
}

/* for string->keyword & symbol->keyword */
/* GC: assumes str is rooted */
TValue kkeyword_new_str(klisp_State *K, TValue str)
{
    return kkeyword_new_bs(K, kstring_buf(str), kstring_size(str));
}

bool kkeywordp(TValue obj) { return ttiskeyword(obj); }
