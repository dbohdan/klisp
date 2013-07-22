/*
** ktable.c
** Kernel Hashtables
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: This is almost textually from lua.
** Parts that don't apply, or don't apply yet to klisp are in comments.
** In klisp arrays are indexed from 0, (while in Lua they are indexed from
** one). So watch out for off by one errors! Andres Navarro
** To indicate a missing entry, klisp uses 'free' instead of 'nil'.
** 'free' is a special type that is unavailable to Kernel programs.
*/

/*
** Implementation of tables (aka arrays, objects, or hash tables).
** Tables keep its elements in two parts: an array part and a hash part.
** Non-negative integer keys are all candidates to be kept in the array
** part. The actual size of the array is the largest `n' such that at
** least half the slots between 0 and n are in use.
** Hash uses a mix of chained scatter table with Brent's variation.
** A main invariant of these tables is that, if an element is not
** in its main position (i.e. the `original' position that its hash gives
** to it), then the colliding element is in its own main position.
** Hence even when the load factor reaches 100%, performance remains good.
*/

#include <math.h>
#include <string.h>

#include "klisp.h"
#include "kgc.h"
#include "kmem.h"
#include "kobject.h"
#include "kstate.h"
#include "ktable.h"
#include "kapplicative.h"
#include "kghelpers.h" /* for eq2p */
#include "kstring.h"

/*
** max size of array part is 2^MAXBITS
*/
#define MAXBITS		26
#define MAXASIZE	(1 << MAXBITS)


#define hashpow2(t,n)         (gnode(t, lmod((n), sizenode(t))))
  
#define hashstr(t,str)  hashpow2(t, (str)->hash)
#define hashsym(t,sym)  hashpow2(t, (sym)->hash)
#define hashboolean(t,p)           hashpow2(t, p? 1 : 0)


/*
** for some types, it is better to avoid modulus by power of 2, as
** they tend to have many 2 factors.
*/
#define hashmod(t,n)	(gnode(t, ((n) % ((sizenode(t)-1)|1))))


#define hashpointer(t,p)	hashmod(t, IntPoint(p))

#define dummynode		(&dummynode_)

static const Node dummynode_ = {
    .i_val = KFREE_,
    .i_key = { .nk = { .this = KFREE_, .next = NULL}} 
};


/*
** hash for klisp numbers
*/
static inline Node *hashfixint (const Table *t, int32_t n) {
    return hashmod(t, (uint32_t) n);
}

/* XXX: this accesses the internal representation of bigints...
   maybe it should be in kbigint.c.
   This may also not be the best hashing for bigints, I just 
   made it up...
*/
static Node *hashbigint (const Table *t, Bigint *b) {
    uint32_t n = (b->sign == 0)? 0 : 1;
    for (uint32_t i = 0; i < b->used; i++) 
        n += b->digits[i];
    
    return hashmod(t, n);
}

/*
** returns the `main' position of an element in a table (that is, the index
** of its hash value)
*/
static Node *mainposition (const Table *t, TValue key) {
    switch (ttype(key)) {
    case K_TNIL:
    case K_TIGNORE:
    case K_TINERT:
    case K_TEOF:
    case K_TFIXINT:
    case K_TEINF: /* infinites have -1 or 1 as ivalues */
        return hashfixint(t, ivalue(key));
    case K_TCHAR:
        return hashfixint(t, chvalue(key));
    case K_TBIGINT:
        return hashbigint(t, tv2bigint(key));
    case K_TBOOLEAN:
        return hashboolean(t, bvalue(key));
    case K_TSTRING:
        if (kstring_immutablep(key))
            return hashstr(t, tv2str(key));
        else /* mutable strings are eq iff they are the same object */
            return hashpointer(t, gcvalue(key));
    case K_TSYMBOL:
        return hashsym(t, tv2sym(key));
    case K_TUSER:
        return hashpointer(t, pvalue(key));
    case K_TAPPLICATIVE: 
        /* applicatives are eq if wrapping the same number of times the
           same applicative, just in case make the hash of an applicative
           the same as the hash of the operative is ultimately wraps */
        while(ttisapplicative(key)) {
            key = kunwrap(key);
        }
        /* fall through */
    default:
        return hashpointer(t, gcvalue(key));
    }
}


/*
** returns the index for `key' if `key' is an appropriate key to live in
** the array part of the table, -1 otherwise.
*/
static int32_t arrayindex (const TValue key) {
    return (ttisfixint(key) && ivalue(key) >= 0)? ivalue(key) : -1;
}


/*
** returns the index of a `key' for table traversals. First goes all
** elements in the array part, then elements in the hash part. The
** beginning of a traversal is signalled by -1.
*/
static int32_t findindex (klisp_State *K, Table *t, TValue key) 
{
    int32_t i;
    if (ttisfree(key)) return -1;  /* first iteration */
    i = arrayindex(key);
    if (0 <= i && i < t->sizearray)  /* is `key' inside array part? */
        return i;  /* yes; that's the index */
    else {
        Node *n = mainposition(t, key);
        do {  /* check whether `key' is somewhere in the chain */
            /* key may be dead already, but it is ok to use it in `next' */
/* klisp: i'm not so sure about this... */
            if (eq2p(K, key2tval(n), key) || 
                (ttype(gkey(n)->this) == K_TDEADKEY && iscollectable(key) &&
                 gcvalue(gkey(n)->this) == gcvalue(key))) { 
                i = (int32_t) (n - gnode(t, 0));  /* key index in hash table */
                /* hash elements are numbered after array ones */
                return i + t->sizearray;
            }
            else n = gnext(n);
        } while (n);
        klispE_throw_simple(K, "invalid key to next");  /* key not found */
        return 0;  /* to avoid warnings */
    }
}

int32_t klispH_next (klisp_State *K, Table *t, TValue *key, TValue *data) 
{
    int32_t i = findindex(K, t, *key);  /* find original element */
    for (i++; i < t->sizearray; i++) {  /* try first array part */
        if (!ttisfree(t->array[i])) {  /* a non-nil value? */
            *key = i2tv(i);
            *data = t->array[i];
            return 1;
        }
    }
    for (i -= t->sizearray; i < sizenode(t); i++) {  /* then hash part */
        if (!ttisfree(gval(gnode(t, i)))) {  /* a non-nil value? */
            *key = key2tval(gnode(t, i));
            *data = gval(gnode(t, i));
            return 1;
        }
    }
    return 0;  /* no more elements */
}


/*
** {=============================================================
** Rehash
** ==============================================================
*/


static int32_t computesizes (int32_t nums[], int32_t *narray) 
{
    int32_t i;
    int32_t twotoi;  /* 2^i */
    int32_t a = 0;  /* number of elements smaller than 2^i */
    int32_t na = 0;  /* number of elements to go to array part */
    int32_t n = 0;  /* optimal size for array part */
    for (i = 0, twotoi = 1; twotoi/2 < *narray; i++, twotoi *= 2) {
        if (nums[i] > 0) {
            a += nums[i];
            if (a > twotoi/2) {  /* more than half elements present? */
                n = twotoi;  /* optimal size (till now) */
                na = a;  /* all elements smaller than n will go to array part */
            }
        }
        if (a == *narray) break;  /* all elements already counted */
    }
    *narray = n;
    klisp_assert(*narray/2 <= na && na <= *narray);
    return na;
}


static int32_t countint (const TValue key, int32_t *nums) 
{
    int32_t k = arrayindex(key);
    if (0 < k && k <= MAXASIZE) {  /* is `key' an appropriate array index? */
        nums[ceillog2(k)]++;  /* count as such */
        return 1;
    }
    else
        return 0;
}


static int32_t numusearray (const Table *t, int32_t *nums) 
{
    int32_t lg;
    int32_t ttlg;  /* 2^lg */
    int32_t ause = 0;  /* summation of `nums' */
    int32_t i = 1;  /* count to traverse all array keys */
    for (lg=0, ttlg=1; lg<=MAXBITS; lg++, ttlg*=2) {  /* for each slice */
        int32_t lc = 0;  /* counter */
        int32_t lim = ttlg;
        if (lim > t->sizearray) {
            lim = t->sizearray;  /* adjust upper limit */
            if (i > lim)
                break;  /* no more elements to count */
        }
        /* count elements in range (2^(lg-1), 2^lg] */
        for (; i <= lim; i++) {
            if (!ttisfree(t->array[i-1]))
                lc++;
        }
        nums[lg] += lc;
        ause += lc;
    }
    return ause;
}


static int32_t numusehash (const Table *t, int32_t *nums, int32_t *pnasize) 
{
    int32_t totaluse = 0;  /* total number of elements */
    int32_t ause = 0;  /* summation of `nums' */
    int32_t i = sizenode(t);
    while (i--) {
        Node *n = &t->node[i];
        if (!ttisfree(gval(n))) {
            ause += countint(key2tval(n), nums);
            totaluse++;
        }
    }
    *pnasize += ause;
    return totaluse;
}


static void setarrayvector (klisp_State *K, Table *t, int32_t size) 
{
    int32_t i;
    klispM_reallocvector(K, t->array, t->sizearray, size, TValue);
    for (i=t->sizearray; i<size; i++)
        t->array[i] = KFREE;
    t->sizearray = size;
}


static void setnodevector (klisp_State *K, Table *t, int32_t size) 
{
    int32_t lsize;
    if (size == 0) {  /* no elements to hash part? */
        t->node = cast(Node *, dummynode);  /* use common `dummynode' */
        lsize = 0;
    }
    else {
        int32_t i;
        lsize = ceillog2(size);
        if (lsize > MAXBITS)
            klispE_throw_simple(K, "table overflow");
        size = twoto(lsize);
        t->node = klispM_newvector(K, size, Node);
        for (i=0; i<size; i++) {
            Node *n = gnode(t, i);
            gnext(n) = NULL;
            gkey(n)->this = KFREE;
            gval(n) = KFREE;
        }
    }
    t->lsizenode = (uint8_t) (lsize);
    t->lastfree = gnode(t, size);  /* all positions are free */
}


static void resize (klisp_State *K, Table *t, int32_t nasize, int32_t nhsize) 
{
    int32_t i;
    int32_t oldasize = t->sizearray;
    int32_t oldhsize = t->lsizenode;
    Node *nold = t->node;  /* save old hash ... */
    if (nasize > oldasize)  /* array part must grow? */
        setarrayvector(K, t, nasize);
    /* create new hash part with appropriate size */
    setnodevector(K, t, nhsize);  
    if (nasize < oldasize) {  /* array part must shrink? */
        t->sizearray = nasize;
        /* re-insert elements from vanishing slice */
        for (i=nasize; i<oldasize; i++) {
            if (!ttisfree(t->array[i])) {
                TValue v = t->array[i];
                *klispH_setfixint(K, t, i) = v;
                checkliveness(G(K), v);
            }
        }
        /* shrink array */
        klispM_reallocvector(K, t->array, oldasize, nasize, TValue);
    }
    /* re-insert elements from hash part */
    for (i = twoto(oldhsize) - 1; i >= 0; i--) {
        Node *old = nold+i;
        if (!ttisfree(gval(old))) {
            TValue v = gval(old);
            *klispH_set(K, t, key2tval(old)) = v;
            checkliveness(G(K), v);
        }
    }
    if (nold != dummynode)
        klispM_freearray(K, nold, twoto(oldhsize), Node);  /* free old array */
}


void klispH_resizearray (klisp_State *K, Table *t, int32_t nasize) 
{
    int32_t nsize = (t->node == dummynode) ? 0 : sizenode(t);
    resize(K, t, nasize, nsize);
}


static void rehash (klisp_State *K, Table *t, const TValue ek) {
    int32_t nasize, na;
    int32_t nums[MAXBITS+1];  /* nums[i] = number of keys between 2^(i-1) and 2^i */
    int32_t i;
    int32_t totaluse;
    for (i=0; i<=MAXBITS; i++) nums[i] = 0;  /* reset counts */
    nasize = numusearray(t, nums);  /* count keys in array part */
    totaluse = nasize;  /* all those keys are integer keys */
    totaluse += numusehash(t, nums, &nasize);  /* count keys in hash part */
    /* count extra key */
    nasize += countint(ek, nums);
    totaluse++;
    /* compute new size for array part */
    na = computesizes(nums, &nasize);
    /* resize the table to new computed sizes */
    resize(K, t, nasize, totaluse - na);
}



/*
** }=============================================================
*/

/* wflags should be either or both of K_FLAG_WEAK_KEYS or K_FLAG_WEAK VALUES */
TValue klispH_new (klisp_State *K, int32_t narray, int32_t nhash, 
                   int32_t wflags)  
{
    klisp_assert((wflags & (K_FLAG_WEAK_KEYS | K_FLAG_WEAK_VALUES)) ==
                 wflags);
    Table *t = klispM_new(K, Table);
    klispC_link(K, (GCObject *) t, K_TTABLE, wflags);
    /* temporary values (kept only if some malloc fails) */
    t->array = NULL;
    t->sizearray = 0;
    t->lsizenode = 0;
    t->node = cast(Node *, dummynode);
    /* root in case gc is run while allocating array or nodes */
    TValue tv_t = gc2table(t);
    krooted_tvs_push(K, tv_t);

    setarrayvector(K, t, narray);
    setnodevector(K, t, nhash);
    krooted_tvs_pop(K);
    return tv_t;
}


void klispH_free (klisp_State *K, Table *t) 
{
    if (t->node != dummynode)
        klispM_freearray(K, t->node, sizenode(t), Node);
    klispM_freearray(K, t->array, t->sizearray, TValue);
    klispM_free(K, t);
}


static Node *getfreepos (Table *t) 
{
    while (t->lastfree-- > t->node) {
        if (ttisfree(gkey(t->lastfree)->this))
            return t->lastfree;
    }
    return NULL;  /* could not find a free place */
}


/*
** inserts a new key into a hash table; first, check whether key's main 
** position is free. If not, check whether colliding node is in its main 
** position or not: if it is not, move colliding node to an empty place and 
** put new key in its main position; otherwise (colliding node is in its main 
** position), new key goes to an empty position. 
*/
static TValue *newkey (klisp_State *K, Table *t, TValue key) 
{
    Node *mp = mainposition(t, key);
    if (!ttisfree(gval(mp)) || mp == dummynode) {
        Node *othern;
        Node *n = getfreepos(t);  /* get a free place */
        if (n == NULL) {  /* cannot find a free place? */
            rehash(K, t, key);  /* grow table */
            return klispH_set(K, t, key);  /* re-insert key into grown table */
        }
        klisp_assert(n != dummynode);
        othern = mainposition(t, key2tval(mp));
        if (othern != mp) {  /* is colliding node out of its main position? */
            /* yes; move colliding node into free position */
            while (gnext(othern) != mp) othern = gnext(othern);  /* find previous */
            gnext(othern) = n;  /* redo the chain with `n' in place of `mp' */
            *n = *mp;  /* copy colliding node into free pos. (mp->next also goes) */
            gnext(mp) = NULL;  /* now `mp' is free */
            gval(mp) = KFREE;
        } else {  /* colliding node is in its own main position */
            /* new node will go into free position */
            gnext(n) = gnext(mp);  /* chain new position */
            gnext(mp) = n;
            mp = n;
        }
    }
    gkey(mp)->this = key;
    klispC_barriert(K, t, key);
    klisp_assert(ttisfree(gval(mp)));
    return &gval(mp);
}


/*
** search function for integers
*/
const TValue *klispH_getfixint (Table *t, int32_t key) 
{
    if (key >= 0 && key < t->sizearray)
        return &t->array[key];
    else {
        Node *n = hashfixint(t, key);
        do {  /* check whether `key' is somewhere in the chain */
            if (ttisfixint(gkey(n)->this) && ivalue(gkey(n)->this) == key)
                return &gval(n);  /* that's it */
            else n = gnext(n);
        } while (n);
        return &kfree;
    }
}


/*
** search function for immutable strings
*/
const TValue *klispH_getstr (Table *t, String *key) {
    klisp_assert(kstring_immutablep(gc2str(key)));
    Node *n = hashstr(t, key);
    do {  /* check whether `key' is somewhere in the chain */
        if (ttisstring(gkey(n)->this) && tv2str(gkey(n)->this) == key)
            return &gval(n);  /* that's it */
        else n = gnext(n);
    } while (n);
    return &kfree;
}

/*
** search function for symbol
*/
const TValue *klispH_getsym (Table *t, Symbol *key) {
    Node *n = hashsym(t, key);
    TValue tv_key = gc2sym(key);
    do {  /* check whether `key' is somewhere in the chain */
        if (ttissymbol(gkey(n)->this) && 
            tv_sym_equal(gkey(n)->this, tv_key))
            return &gval(n);  /* that's it */
        else n = gnext(n);
    } while (n);
    return &kfree;
}


/*
** main search function
*/
const TValue *klispH_get (Table *t, TValue key) 
{
    switch (ttype(key)) {
    case K_TFREE: return &kfree;
    case K_TSYMBOL: return klispH_getsym(t, tv2sym(key));
    case K_TFIXINT: return klispH_getfixint(t, ivalue(key));
    case K_TSTRING: 
        if (kstring_immutablep(key))
            return klispH_getstr(t, tv2str(key));
        /* else fall through */
    default: {
        Node *n = mainposition(t, key);
        do {  /* check whether `key' is somewhere in the chain */
            /* XXX: for some reason eq2p takes klisp_State but 
               doesn't use it */
            if (eq2p((klisp_State *)NULL, key2tval(n), key))
                return &gval(n);  /* that's it */
            else n = gnext(n);
        } while (n);
        return &kfree;
    }
    }
}


TValue *klispH_set (klisp_State *K, Table *t, TValue key) 
{
    const TValue *p = klispH_get(t, key);
    if (p != &kfree)
        return cast(TValue *, p);
    else {
        if (ttisfree(key)) 
            klispE_throw_simple(K, "table index is free");
/*
  else if (ttisnumber(key) && luai_numisnan(nvalue(key)))
  luaG_runerror(L, "table index is NaN");
*/
        return newkey(K, t, key);
    }
}


TValue *klispH_setfixint (klisp_State *K, Table *t, int32_t key) 
{
    const TValue *p = klispH_getfixint(t, key);
    if (p != &kfree)
        return cast(TValue *, p);
    else 
        return newkey(K, t, i2tv(key));
}


TValue *klispH_setstr (klisp_State *K, Table *t, String *key)
{
    klisp_assert(kstring_immutablep(gc2str(key)));
    const TValue *p = klispH_getstr(t, key);
    if (p != &kfree)
        return cast(TValue *, p);
    else {
        return newkey(K, t, gc2str(key));
    }
}


TValue *klispH_setsym (klisp_State *K, Table *t, Symbol *key)
{
    const TValue *p = klispH_getsym(t, key);
    if (p != &kfree)
        return cast(TValue *, p);
    else {
        return newkey(K, t, gc2sym(key));
    }
}


/* klisp: Untested, may have off by one errors, check before using */
static int32_t unbound_search (Table *t, int32_t j) {
    int32_t i = j;  /* i -1 or a present index */
    j++;
    /* find `i' and `j' such that i is present and j is not */
    while (!ttisfree(*klispH_getfixint(t, j))) {
        i = j;
        if (j <= (INT32_MAX - i) / 2)
            j *= 2;
        else {  /* overflow? */
            /* table was built with bad purposes: resort to linear search */
            i = 0;
            while (!ttisfree(*klispH_getfixint(t, i))) i++;
            return i-1;
        }
    }
    /* now do a binary search between them */
    while (j - i > 1) {
        int32_t m = (i+j)/2;
        if (ttisfree(*klispH_getfixint(t, m))) j = m;
        else i = m;
    }
    return i;
}


/*
** Try to find a boundary in table `t'. A `boundary' is an integer index
** such that t[i] is non-nil and t[i+1] is nil (and 0 if t[1] is nil).
** klisp: in klisp that indexes are from zero, this returns -1 if t[0] is nil 
** also klisp uses free instead of nil
*/
int32_t klispH_getn (Table *t) {
    int32_t j = t->sizearray - 1;
    if (j >= 0 && ttisfree(t->array[j])) {
        /* there is a boundary in the array part: (binary) search for it */
        int32_t i = -1;
        while (j - i > 1) {
            int32_t m = (i+j)/2;
            if (ttisfree(t->array[m])) j = m;
            else i = m;
        }
        return i;
    }
    /* else must find a boundary in hash part */
    else if (t->node == dummynode)  /* hash part is empty? */
        return j;  /* that is easy... */
    else return unbound_search(t, j);
}

/* Return number of used elements in the hashtable. Code copied
 * from rehash(). */

int32_t klispH_numuse(Table *t)
{
    int32_t nasize;
    int32_t nums[MAXBITS+1];  /* nums[i] = number of keys between 2^(i-1) and 2^i */
    int32_t i;
    int32_t totaluse;
    for (i=0; i<=MAXBITS; i++) nums[i] = 0;  /* reset counts */
    nasize = numusearray(t, nums);  /* count keys in array part */
    totaluse = nasize;  /* all those keys are integer keys */
    totaluse += numusehash(t, nums, &nasize);  /* count keys in hash part */
    return totaluse;
}

bool ktablep(TValue obj)
{
    return ttistable(obj);
}
