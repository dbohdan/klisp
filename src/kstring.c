/*
** kstring.c
** Kernel Strings
** See Copyright Notice in klisp.h
*/

/* SOURCE NOTE: the string table & hashing code is from lua */

#include <string.h>
#include <stdbool.h>

#include "kstring.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

/* for immutable string/symbols table */
void klispS_resize (klisp_State *K, int32_t newsize)
{
    GCObject **newhash;
    stringtable *tb;
    int32_t i;
    if (K->gcstate == GCSsweepstring)
	return;  /* cannot resize during GC traverse */
    newhash = klispM_newvector(K, newsize, GCObject *);
    tb = &K->strt;
    for (i = 0; i < newsize; i++) newhash[i] = NULL;
    /* rehash */
    for (i = 0; i < tb->size; i++) {
	GCObject *p = tb->hash[i];
	while (p) {  /* for each node in the list */
	    /* imm string & symbols aren't chained with all other
	       objs, but with each other in strt */
	    GCObject *next = p->gch.next;  /* save next */
	    
	    uint32_t h = 0;

	    if (p->gch.tt == K_TSYMBOL) {
		h = ((Symbol *) p)->hash;
	    } else if (p->gch.tt == K_TSTRING) {
		h = ((String *) p)->hash;
	    } else {
		klisp_assert(0);
	    }

	    int32_t h1 = lmod(h, newsize);  /* new position */
	    klisp_assert((int32_t) (h%newsize) == lmod(h, newsize));
	    p->gch.next = newhash[h1];  /* chain it */
	    newhash[h1] = p;
	    p = next;
	}
    }
    klispM_freearray(K, tb->hash, tb->size, GCObject *);
    tb->size = newsize;
    tb->hash = newhash;
}

/* General constructor for strings */
TValue kstring_new_bs_g(klisp_State *K, bool m, const char *buf, 
			uint32_t size)
{
    return m? kstring_new_bs(K, buf, size) :
	kstring_new_bs_imm(K, buf, size);
}

/* 
** Constructors for immutable strings
*/

/* main constructor for immutable strings */
TValue kstring_new_bs_imm(klisp_State *K, const char *buf, uint32_t size)
{
    /* first check to see if it's in the stringtable */
    GCObject *o;
    uint32_t h = size; /* seed */
    size_t step = (size>>5)+1; /* if string is too long, don't hash all 
			       its chars */
    size_t size1;
    for (size1 = size; size1 >= step; size1 -= step)  /* compute hash */
	h = h ^ ((h<<5)+(h>>2)+ ((unsigned char) buf[size1-1]));

    for (o = K->strt.hash[lmod(h, K->strt.size)];
	            o != NULL; o = o->gch.next) {
	String *ts = NULL;
	if (o->gch.tt == K_TSTRING) {
	    ts = (String *) o;
	} else if (o->gch.tt == K_TSYMBOL) {
	    continue; 
	} else {
	    klisp_assert(0); /* only symbols and immutable strings */
	}
	if (ts->size == size && (memcmp(buf, ts->b, size) == 0)) {
	    /* string may be dead */
	    if (isdead(K, o)) changewhite(o);
	    return gc2str(o);
	}
    } 

    /* If it exits the loop, it means it wasn't found, hash is still in h */
    /* REFACTOR: move all of these to a new function */
    String *new_str;

    if (size+1 > (SIZE_MAX - sizeof(String)))
	klispM_toobig(K);

    new_str = (String *) klispM_malloc(K, sizeof(String) + size + 1);

    /* header + gc_fields */
    /* can't use klispC_link, because strings use the next pointer
       differently */
    new_str->gct = klispC_white(K);
    new_str->tt = K_TSTRING;
    new_str->kflags = K_FLAG_IMMUTABLE;
    new_str->si = NULL;
    /* string specific fields */
    new_str->hash = h;
    new_str->mark = KFALSE;
    new_str->size = size;
    if (size != 0) {
	memcpy(new_str->b, buf, size);
    }
    new_str->b[size] = '\0'; /* final 0 for printing */

    /* add to the string/symbol table (and link it) */
    stringtable *tb;
    tb = &K->strt;
    h = lmod(h, tb->size);
    new_str->next = tb->hash[h];  /* chain new entry */
    tb->hash[h] = (GCObject *)(new_str);
    tb->nuse++;
    TValue ret_tv = gc2str(new_str);
    if (tb->nuse > ((uint32_t) tb->size) && tb->size <= INT32_MAX / 2) {
	krooted_tvs_push(K, ret_tv); /* save in case of gc */
	klispS_resize(K, tb->size*2);  /* too crowded */
	krooted_tvs_pop(K);
    }
    
    return ret_tv;
}

/* with just buffer, no embedded '\0's */
TValue kstring_new_b_imm(klisp_State *K, const char *buf)
{
    return (kstring_new_bs_imm(K, buf, strlen(buf)));
}

/* 
** Constructors for mutable strings
*/

/* main constructor for mutable strings */
/* with just size */
TValue kstring_new_s(klisp_State *K, uint32_t size)
{
    String *new_str;

    if (size == 0) {
	return K->empty_string;
    }

    new_str = klispM_malloc(K, sizeof(String) + size + 1);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_str, K_TSTRING, 0);

    /* string specific fields */
    new_str->hash = 0; /* unimportant for mutable strings */
    new_str->mark = KFALSE;
    new_str->size = size;

    /* the buffer is initialized elsewhere */

    /* NOTE: all string end with a '\0' for convenience in printing 
       even if they have embedded '\0's */
    new_str->b[size] = '\0';

    return gc2str(new_str);
}

/* with buffer & size */
TValue kstring_new_bs(klisp_State *K, const char *buf, uint32_t size)
{
    TValue new_str = kstring_new_s(K, size);
    memcpy(kstring_buf(new_str), buf, size);
    return new_str;
}

/* with buffer but no size, no embedded '\0's */
TValue kstring_new_b(klisp_State *K, const char *buf)
{
    return (kstring_new_bs(K, buf, strlen(buf)));
}

/* with size and fill char */
TValue kstring_new_sf(klisp_State *K, uint32_t size, char fill)
{
    TValue new_str = kstring_new_s(K, size);
    memset(kstring_buf(new_str), fill, size);
    return new_str;
}

/* both obj1 and obj2 should be strings */
bool kstring_equalp(TValue obj1, TValue obj2)
{
    klisp_assert(ttisstring(obj1) && ttisstring(obj2));

    String *str1 = tv2str(obj1);
    String *str2 = tv2str(obj2);

    if (str1->size == str2->size) {
	return (str1->size == 0) ||
	    (memcmp(str1->b, str2->b, str1->size) == 0);
    } else {
	return false;
    }
}

bool kstringp(TValue obj) { return ttisstring(obj); }
