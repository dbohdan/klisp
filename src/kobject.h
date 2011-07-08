/*
** kobject.h
** Type definitions for Kernel Objects
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: While the tagging system comes from Mozilla TraceMonkey,
** no code from TraceMonkey was used.
** The general structure, names and comments of this file follow the 
** scheme of Lua. 
*/

/*
** TODO:
**
** - #ifdef for little/big endian (for now, only little endian)
**    Should be careful with endianness of floating point numbers too,
**    as they don't necessarily match the endianness of other values
** - #ifdef of 32/64 bits (for now, only 32 bits)
**    See TraceMonkey and _funderscore comments on reddit
**    for 64 bits implementation ideas
**    47 bits should be enough for pointers (see Canonical Form Addresses)
** - #ifdef for alignment/packing info (for now, only gcc)
**
*/

#ifndef kobject_h
#define kobject_h

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "klimits.h"
#include "klispconf.h"

/*
** Union of all collectible objects
*/
typedef union GCObject GCObject;

/*
** Common Header for all collectible objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader GCObject *next; uint8_t tt; uint8_t kflags; \
    uint16_t gct; GCObject *si; GCObject *gclist;
    
/* NOTE: the gc flags are called marked in lua, but we reserve that them
   for marks used in cycle traversal. The field kflags is also missing
   from lua, they serve as compact bool fields for certain types */

/* 
** NOTE: this is next pointer comes from lua. This is a byproduct of the 
** lua allocator. Because objects come from an arbitrary allocator, they
** can't be assumed to be contiguous; but in the sweep phase of garbage 
** collection there has to be a way to iterate over all allocated objects
** and that is the function of the next pointer: for storing the white
** list. Upon allocation objects are added to this white list, all linked
** together by a succession of next pointers starting in a field of the
** state struct. Likewise, during the tracing phase, gray objects are linked
** by means of the gclist pointer. Technically this is necessary only for
** objects that have references, but in klisp all objects except strings
** have them so it is easier to just put it here. 
*/

/* 
** MAYBE/REFACTOR: other way to do it would be to have a packed GCHeader 
** struct inside each object, but would have to change all references to 
** header objects from 'obj.*' to 'obj.h.*', or something like that. I 
** think the next C standard (C1X at this point) allows the use of
** anonymous inner struct and unions for this use case
*/

/*
** Common header in struct form
*/
typedef struct __attribute__ ((__packed__)) GCheader {
  CommonHeader;
} GCheader;

/*
** Tags: Types & Flags
**
** Nan Boxing: Tagged values in 64 bits (for 32 bit systems)
** All Values except doubles are encoded as double precision NaNs
** There is one canonical NaN(?maybe none?) that is used through the 
** interpreter and all remaining NaNs are used to encode the rest of 
** the types (other than double)
** Canonical NaN(?): (0)(111 1111 1111) 1000  0000 0000 0000 0000 32(0)
** Infinities(?): s(111 1111 1111) 0000  0000 0000 0000 0000 32(0)
** Tagged values: (0)(111 1111 1111) 1111  tttt tttt tttt tttt 32(v)
** So all tags start with 0x7fff which leaves us 16 bits for the 
** tag proper.
** The tag consist of an 8 bit flag part and an 8 bit type part
** so tttt tttt tttt tttt is actually ffff ffff tttt tttt
** This gives us 256 types and as many as 8 flags per type.
*/

/* TODO eliminate flags */

/*
** Macros for manipulating tags directly
*/
#define K_TAG_TAGGED 0x7fff0000
#define K_TAG_BASE_MASK 0x7fff0000
#define K_TAG_BASE_TYPE_MASK 0x7fff00ff

#define K_TAG_FLAG(t) (((t) >> 8) & 0xff)
#define K_TAG_TYPE(t) ((t) & 0xff)
#define K_TAG_BASE(t) ((t) & K_TAG_BASE_MASK)
#define K_TAG_BASE_TYPE(t) ((t) & K_TAG_BASE_TYPE_MASK)

/*
** RATIONALE:
** Number types are first and ordered to allow easy switch statements
** in arithmetic operators. The ones marked with (?) are still in
** consideration for separate type tags.
** They are in order: fixed width integers, arbitrary integers, 
** fixed width rationals, arbitrary rationals, exact infinities,
** inexact reals (doubles) and infinities(?) and real with no primary 
** values(NaN)(?), bounded reals (heap allocated), inexact infinities(?), 
** real with no primary value (?),
** complex numbers (heap allocated) 
*/

/* LUA NOTE: In Lua the corresponding defines are in lua.h */
#define K_TFIXINT       0
#define K_TBIGINT       1
#define K_TFIXRAT       2
#define K_TBIGRAT       3
#define K_TDOUBLE       4
#define K_TBDOUBLE      5
#define K_TEINF         6
#define K_TIINF         7
#define K_TRWNPV        8
#define K_TCOMPLEX      9
#define K_TUNDEFINED    10

#define K_TNIL 		20
#define K_TIGNORE 	21
#define K_TINERT 	22
#define K_TEOF 		23
#define K_TBOOLEAN 	24
#define K_TCHAR 	25
#define K_TFREE 	26 /* this is used instead of lua nil in tables */
/* user pointer */
#define K_TUSER 	29

#define K_TPAIR        	30
#define K_TSTRING	31
#define K_TSYMBOL	32
#define K_TENVIRONMENT  33
#define K_TCONTINUATION 34
#define K_TOPERATIVE    35
#define K_TAPPLICATIVE  36
#define K_TENCAPSULATION 37
#define K_TPROMISE      38
#define K_TPORT         39
#define K_TTABLE        40
#define K_TERROR        41
#define K_TBLOB         42

/* for tables */
#define K_TDEADKEY        60

/* this is used to test for numbers, as returned by ttype */
#define K_LAST_NUMBER_TYPE K_TUNDEFINED

/* this is used to if the object is collectable */
#define K_FIRST_GC_TYPE K_TPAIR

#define K_MAKE_VTAG(t) (K_TAG_TAGGED | (t))

/*
** TODO: 
**
** - decide if inexact infinities and reals with no
**    primary values are included in K_TDOUBLE
** - All types except complexs, bounded reals and fixrats 
*/
#define K_TAG_FIXINT	K_MAKE_VTAG(K_TFIXINT)
#define K_TAG_BIGINT	K_MAKE_VTAG(K_TBIGINT)
#define K_TAG_BIGRAT	K_MAKE_VTAG(K_TBIGRAT)
#define K_TAG_EINF	K_MAKE_VTAG(K_TEINF)
#define K_TAG_IINF	K_MAKE_VTAG(K_TIINF)
#define K_TAG_RWNPV	K_MAKE_VTAG(K_TRWNPV)
#define K_TAG_UNDEFINED	K_MAKE_VTAG(K_TUNDEFINED)

#define K_TAG_NIL	K_MAKE_VTAG(K_TNIL)
#define K_TAG_IGNORE	K_MAKE_VTAG(K_TIGNORE)
#define K_TAG_INERT	K_MAKE_VTAG(K_TINERT)
#define K_TAG_EOF	K_MAKE_VTAG(K_TEOF)
#define K_TAG_BOOLEAN	K_MAKE_VTAG(K_TBOOLEAN)
#define K_TAG_CHAR	K_MAKE_VTAG(K_TCHAR)
#define K_TAG_FREE	K_MAKE_VTAG(K_TFREE)
#define K_TAG_DEADKEY	K_MAKE_VTAG(K_TDEADKEY)

#define K_TAG_USER	K_MAKE_VTAG(K_TUSER)

#define K_TAG_PAIR K_MAKE_VTAG(K_TPAIR)
#define K_TAG_STRING K_MAKE_VTAG(K_TSTRING)
#define K_TAG_SYMBOL K_MAKE_VTAG(K_TSYMBOL)

#define K_TAG_SYMBOL K_MAKE_VTAG(K_TSYMBOL)
#define K_TAG_ENVIRONMENT K_MAKE_VTAG(K_TENVIRONMENT)
#define K_TAG_CONTINUATION K_MAKE_VTAG(K_TCONTINUATION)
#define K_TAG_OPERATIVE K_MAKE_VTAG(K_TOPERATIVE)
#define K_TAG_APPLICATIVE K_MAKE_VTAG(K_TAPPLICATIVE)
#define K_TAG_ENCAPSULATION K_MAKE_VTAG(K_TENCAPSULATION)
#define K_TAG_PROMISE K_MAKE_VTAG(K_TPROMISE)
#define K_TAG_PORT K_MAKE_VTAG(K_TPORT)
#define K_TAG_TABLE K_MAKE_VTAG(K_TTABLE)
#define K_TAG_ERROR K_MAKE_VTAG(K_TERROR)
#define K_TAG_BLOB K_MAKE_VTAG(K_TBLOB)


/*
** Macros to test types
*/

/* NOTE: This is intended for use in switch statements */
#define ttype(o) ({ TValue tto_ = (o);			\
	    ttisdouble(tto_)? K_TDOUBLE : ttype_(tto_); })

/* This is intended for internal use below. DON'T USE OUTSIDE THIS FILE */
#define ttag(o) ((o).tv.t)
#define ttype_(o) (K_TAG_TYPE(ttag(o)))
/* NOTE: not used for now */
#define tflag_(o) (K_TAG_FLAG(ttag(o)))
#define tbasetype_(o) (K_TAG_BASE_TYPE(ttag(o)))

/* Simple types (value in TValue struct) */
#define ttisfixint(o)	(tbasetype_(o) == K_TAG_FIXINT)
#define ttisbigint(o)	(tbasetype_(o) == K_TAG_BIGINT)
#define ttiseinteger(o_) ({ int32_t t_ = tbasetype_(o_); \
	    t_ == K_TAG_FIXINT || t_ == K_TAG_BIGINT;})
#define ttisinteger(o) ({ TValue o__ = (o);				\
	    (ttiseinteger(o__) ||					\
	     (ttisdouble(o__) && (floor(dvalue(o__)) == dvalue(o__))));})
#define ttisbigrat(o)	(tbasetype_(o) == K_TAG_BIGRAT)
#define ttisrational(o_)				\
    ({ TValue t_ = o_;					\
	(ttype(t_) <= K_TBIGRAT) || ttisdouble(t_); })
#define ttisdouble(o)	((ttag(o) & K_TAG_BASE_MASK) != K_TAG_TAGGED)
#define ttisreal(o) (ttype(o) < K_TCOMPLEX)
#define ttisexact(o_)					\
    ({ TValue t_ = o_;					\
	(ttiseinf(t_) || ttype(t_) <= K_TBIGRAT); })
/* MAYBE this is ugly..., maybe add exact/inexact flag, real, rational flag */
#define ttisinexact(o_)					\
    ({ TValue t_ = o_;					\
	(ttisundef(t_) || ttisdouble(t_) || ttisrwnpv(t_) || ttisiinf(t_)); })
/* For now, all inexact numbers are not robust and have -inf & +inf bounds */
#define ttisrobust(o)	(ttisexact(o))
#define ttisnumber(o) (ttype(o) <= K_LAST_NUMBER_TYPE)
#define ttiseinf(o)	(tbasetype_(o) == K_TAG_EINF)
#define ttisiinf(o)	(tbasetype_(o) == K_TAG_IINF)
#define ttisinf(o_)				\
    ({ TValue t_ = o_;				\
	(ttiseinf(t_) || ttisiinf(t_)); })
#define ttisrwnpv(o)	(tbasetype_(o) == K_TAG_RWNPV)
#define ttisundef(o)	(tbasetype_(o) == K_TAG_UNDEFINED)
#define ttisnwnpv(o_)				     \
    ({ TValue t_ = o_;				     \
	(ttisundef(t_) || ttisrwnpv(t_)); })

#define ttisnil(o)	(tbasetype_(o) == K_TAG_NIL)
#define ttisignore(o)	(tbasetype_(o) == K_TAG_IGNORE)
#define ttisinert(o)	(tbasetype_(o) == K_TAG_INERT)
#define ttiseof(o)	(tbasetype_(o) == K_TAG_EOF)
#define ttisboolean(o)	(tbasetype_(o) == K_TAG_BOOLEAN)
#define ttischar(o)	(tbasetype_(o) == K_TAG_CHAR)
#define ttisfree(o)	(tbasetype_(o) == K_TAG_FREE)

/* Complex types (value in heap), 
   (bigints, rationals, etc could be collectable)
   maybe we should use a better way for this, to speed up checks, maybe use
   a flag? */
#define iscollectable(o)  ({ uint8_t t = ttype(o);			\
	    (t == K_TBIGINT || t == K_TBIGRAT || t >= K_FIRST_GC_TYPE); })

#define ttisstring(o)	(tbasetype_(o) == K_TAG_STRING)
#define ttissymbol(o)	(tbasetype_(o) == K_TAG_SYMBOL)
#define ttispair(o)	(tbasetype_(o) == K_TAG_PAIR)
#define ttisoperative(o) (tbasetype_(o) == K_TAG_OPERATIVE)
#define ttisapplicative(o) (tbasetype_(o) == K_TAG_APPLICATIVE)
#define ttiscombiner(o_) ({ int32_t t_ = tbasetype_(o_); \
	    t_ == K_TAG_OPERATIVE || t_ == K_TAG_APPLICATIVE;})
#define ttisenvironment(o) (tbasetype_(o) == K_TAG_ENVIRONMENT)
#define ttiscontinuation(o) (tbasetype_(o) == K_TAG_CONTINUATION)
#define ttisencapsulation(o) (tbasetype_(o) == K_TAG_ENCAPSULATION)
#define ttispromise(o) (tbasetype_(o) == K_TAG_PROMISE)
#define ttisport(o) (tbasetype_(o) == K_TAG_PORT)
#define ttistable(o) (tbasetype_(o) == K_TAG_TABLE)
#define ttiserror(o) (tbasetype_(o) == K_TAG_ERROR)
#define ttisblob(o) (tbasetype_(o) == K_TAG_BLOB)

/* macros to easily check boolean values */
#define kis_true(o_) (tv_equal((o_), KTRUE))
#define kis_false(o_) (tv_equal((o_), KFALSE))
/* unsafe, doesn't check type */
#define knegp(o_) (kis_true(o_)? KFALSE : KTRUE)

/*
** Union of all Kernel non heap-allocated values (except doubles)
*/
typedef union {
    bool b;
    int32_t i;
    char ch;
    GCObject *gc; 
    void *p; 
    /* ... */
} Value;

/*
** All Kernel non heap-allocated values (except doubles) tagged
*/
typedef struct __attribute__ ((__packed__)) InnerTV {
    Value v;
    uint32_t t;
} InnerTV;

/*
** Union of all Kernel non heap-allocated values
*/
typedef __attribute__((aligned (8))) union {
    double d;
    InnerTV tv;
    int64_t raw;
} TValue;

/*
** Individual heap-allocated values
*/

typedef struct __attribute__ ((__packed__)) {
    CommonHeader; 
/* These are all from IMath (XXX: find a way to use mp_types directly) */
    uint32_t single;
    uint32_t *digits;
    uint32_t alloc;
    uint32_t used;
    unsigned char sign;
} Bigint;

/* NOTE: Notice that both num and den aren't pointers, so, in general, to get 
   the denominator or numerator we have to make a copy, this comes from IMath.
   If written for klisp I would have put pointers. Maybe I'll later change it
   but for now minimal ammount of modification to IMath is desired */
typedef struct __attribute__ ((__packed__)) {
    CommonHeader; 
/* This is from IMath */
    Bigint num; /* Numerator         */
    Bigint den; /* Denominator, <> 0 */
} Bigrat;

/* REFACTOR: move these macros somewhere else */
/* NOTE: The use of the intermediate KCONCAT is needed to allow
   expansion of the __LINE__ macro.  */
#define KCONCAT_(a, b) a ## b
#define KCONCAT(a, b) KCONCAT_(a, b)
#define KUNIQUE_NAME(prefix) KCONCAT(prefix, __LINE__ )

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for cycle/sharing aware algorithms */
    TValue car;
    TValue cdr;
} Pair;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader; /* symbols are marked via their strings */
    TValue str; /* could use String * here, but for now... */
    uint32_t hash; /* this is different from the str hash to
		      avoid having both the string and the symbol
		      from always falling in the same bucket */
} Symbol;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for cycle/sharing aware algorithms */
    TValue parents; /* may be (), a list, or a single env */
    TValue bindings; /* alist of (binding . value) or table */
    /* for keyed static vars */
    TValue keyed_node; /* (key . value) pair or KNIL */
    /* this is a different field from parents to jump over non keyed
       envs in the search */
    TValue keyed_parents; /* maybe (), a list, or a single env */
} Environment;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for guarding continuation */
    TValue parent; /* may be () for root continuation */
    TValue comb; /* combiner that created the cont (or #inert) */
    void *fn; /* the function that does the work */
    int32_t extra_size;
    TValue extra[];
} Continuation;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    void *fn; /* the function that does the work */
    int32_t extra_size;
    TValue extra[];
} Operative;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue underlying; /* underlying operative/applicative */
} Applicative;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue key; /* unique pair identifying this type of encapsulation */
    TValue value; /* encapsulated object */
} Encapsulation;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue node; /* pair (exp . maybe-env) */
    /* if maybe-env is nil, then the promise has determined exp,
       otherwise the promise should eval exp in maybe-env when forced 
       It has to be a pair to allow sharing between different promises
       So that determining one determines all the promises that are
       sharing the pair */
} Promise;

/* input/output direction and open/close status are in kflags */
typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue filename;
    /* TEMP: for now source code info is in fixints */
    int32_t row;
    int32_t col;
    FILE *file;
} Port;

/* input/output direction and open/close status are in kflags */

/*
** Hashtables
*/

typedef union TKey {
  struct {
      TValue this; /* different from lua because of the tagging scheme */
      struct Node *next;  /* for chaining */
  } nk;
  TValue tvk;
} TKey;

typedef struct Node {
  TValue i_val;
  TKey i_key;
} Node;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    uint8_t lsizenode;  /* log2 of size of `node' array */
    uint8_t t1padding; 
    uint16_t t2padding; /* to avoid disturbing the alignment */
    TValue *array;  /* array part */
    Node *node;
    Node *lastfree;  /* any free position is before this position */
    int32_t sizearray;  /* size of `array' array */
} Table;

/* The weak flags are in kflags */

/* Errors */
typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue who;  /* either #inert or creating combiner/continuation */
    TValue cont;  /* continuation context */
    TValue msg;  /* string msg */
    TValue irritants;  /* list of extra objs */
} Error;

/* Blobs (binary vectors) */
typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for cycle/sharing aware algorithms */
    uint32_t size;
    int32_t __dummy; /* for alignment to 64 bits */
    char b[]; /* buffer */
} Blob;

/*
** `module' operation for hashing (size is always a power of 2)
*/
#define lmod(s,size) \
	(check_exp((size&(size-1))==0, (cast(int32_t, (s) & ((size)-1)))))


#define twoto(x)	(1<<(x))
#define sizenode(t)	(twoto((t)->lsizenode))

#define ceillog2(x)	(klispO_log2((x)-1) + 1)

int32_t klispO_log2 (uint32_t x);

/* 
** RATIONALE: 
**
** Storing size allows embedded '\0's.
** Note, however, that there are actually size + 1 bytes allocated
** and that b[size] = '\0'. This is useful for printing strings
** 
*/
typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for cycle/sharing aware algorithms */
    uint32_t size; 
    uint32_t hash; /* only used for immutable strings */
    char b[]; /* buffer */
} String;

/* MAYBE: mark fields could be replaced by a hashtable or a bit + a hashtable */

/*
** Common header for markable objects
*/
typedef struct __attribute__ ((__packed__)) {
  CommonHeader;
  TValue mark;
} MGCheader;

/*
** Union of all Kernel heap-allocated values
*/
/* LUA NOTE: In Lua the corresponding union is in lstate.h */
union GCObject {
    GCheader gch;
    MGCheader mgch;
    Pair pair;
    Symbol sym;
    String str;
    Environment env;
    Continuation cont;
    Operative op;
    Applicative app;
    Encapsulation enc;
    Promise prom;
    Port port;
    Table table;
};


/*
** Some constants 
*/
#define KNIL_ {.tv = {.t = K_TAG_NIL, .v = { .i = 0 }}}
#define KINERT_ {.tv = {.t = K_TAG_INERT, .v = { .i = 0 }}}
#define KIGNORE_ {.tv = {.t = K_TAG_IGNORE, .v = { .i = 0 }}}
#define KEOF_ {.tv = {.t = K_TAG_EOF, .v = { .i = 0 }}}
#define KTRUE_ {.tv = {.t = K_TAG_BOOLEAN, .v = { .b = true }}}
#define KFALSE_ {.tv = {.t = K_TAG_BOOLEAN, .v = { .b = false }}}
#define KEPINF_ {.tv = {.t = K_TAG_EINF, .v = { .i = 1 }}}
#define KEMINF_ {.tv = {.t = K_TAG_EINF, .v = { .i = -1 }}}
#define KIPINF_ {.tv = {.t = K_TAG_IINF, .v = { .i = 1 }}}
#define KIMINF_ {.tv = {.t = K_TAG_IINF, .v = { .i = -1 }}}
#define KRWNPV_ {.tv = {.t = K_TAG_RWNPV, .v = { .i = 0 }}}
#define KUNDEF_ {.tv = {.t = K_TAG_UNDEFINED, .v = { .i = 0 }}}
#define KSPACE_ {.tv = {.t = K_TAG_CHAR, .v = { .ch = ' ' }}}
#define KNEWLINE_ {.tv = {.t = K_TAG_CHAR, .v = { .ch = '\n' }}}
#define KFREE_ {.tv = {.t = K_TAG_FREE, .v = { .i = 0 }}}


/* RATIONALE: the ones above can be used in initializers */
#define KNIL ((TValue) KNIL_)
#define KINERT ((TValue) KINERT_)
#define KIGNORE ((TValue) KIGNORE_)
#define KEOF ((TValue) KEOF_)
#define KTRUE ((TValue) KTRUE_)
#define KFALSE ((TValue) KFALSE_)
#define KEPINF ((TValue) KEPINF_)
#define KEMINF ((TValue) KEMINF_)
#define KIPINF ((TValue) KIPINF_)
#define KIMINF ((TValue) KIMINF_)
#define KRWNPV ((TValue) KRWNPV_)
#define KUNDEF ((TValue) KUNDEF_)
#define KSPACE ((TValue) KSPACE_)
#define KNEWLINE ((TValue) KNEWLINE_)
#define KFREE ((TValue) KFREE_)

/* The same constants as global const variables */
const TValue knil;
const TValue kignore;
const TValue kinert;
const TValue keof;
const TValue ktrue;
const TValue kfalse;
const TValue kepinf;
const TValue keminf;
const TValue kipinf;
const TValue kiminf;
const TValue krwnpv;
const TValue kundef;
const TValue kspace;
const TValue knewline;
const TValue kfree;

/* Macros to create TValues of non-heap allocated types (for initializers) */
#define ch2tv_(ch_) {.tv = {.t = K_TAG_CHAR, .v = { .ch = (ch_) }}}
#define i2tv_(i_) {.tv = {.t = K_TAG_FIXINT, .v = { .i = (i_) }}}
#define b2tv_(b_) {.tv = {.t = K_TAG_BOOLEAN, .v = { .b = (b_) }}}
#define p2tv_(p_) {.tv = {.t = K_TAG_USER, .v = { .p = (p_) }}}
#define d2tv_(d_) {.d = d_}
#define ktag_double(d_)							\
    ({ double d__ = d_;							\
	TValue res__;							\
	if (isnan(d__)) res__ = KRWNPV;					\
	else if (isinf(d__)) res__ = (d__ == INFINITY)?			\
				 KIPINF : KIMINF;			\
	/* +0.0 == -0.0 too, but that doesn't hurt */			\
	else if (d__ == -0.0) res__ = d2tv(+0.0);			\
	else res__ = d2tv(d__);						\
	res__;})

/* Macros to create TValues of non-heap allocated types */
#define ch2tv(ch_) ((TValue) ch2tv_(ch_))
#define i2tv(i_) ((TValue) i2tv_(i_))
#define b2tv(b_) ((TValue) b2tv_(b_))
#define p2tv(p_) ((TValue) p2tv_(p_))
#define d2tv(d_) ((TValue) d2tv_(d_))

/* Macros to convert a GCObject * into a tagged value */
/* TODO: add assertions */
/* REFACTOR: change names to bigint2tv, pair2tv, etc */
/* LUA NOTE: the corresponding defines are in lstate.h */
#define gc2tv(t_, o_) ((TValue) {.tv = {.t = (t_),			\
					.v = { .gc = obj2gco(o_)}}})
#define gc2bigint(o_) (gc2tv(K_TAG_BIGINT, o_))
#define gc2bigrat(o_) (gc2tv(K_TAG_BIGRAT, o_))
#define gc2pair(o_) (gc2tv(K_TAG_PAIR, o_))
#define gc2str(o_) (gc2tv(K_TAG_STRING, o_))
#define gc2sym(o_) (gc2tv(K_TAG_SYMBOL, o_))
#define gc2env(o_) (gc2tv(K_TAG_ENVIRONMENT, o_))
#define gc2cont(o_) (gc2tv(K_TAG_CONTINUATION, o_))
#define gc2op(o_) (gc2tv(K_TAG_OPERATIVE, o_))
#define gc2app(o_) (gc2tv(K_TAG_APPLICATIVE, o_))
#define gc2enc(o_) (gc2tv(K_TAG_ENCAPSULATION, o_))
#define gc2prom(o_) (gc2tv(K_TAG_PROMISE, o_))
#define gc2port(o_) (gc2tv(K_TAG_PORT, o_))
#define gc2table(o_) (gc2tv(K_TAG_TABLE, o_))
#define gc2error(o_) (gc2tv(K_TAG_ERROR, o_))
#define gc2blob(o_) (gc2tv(K_TAG_BLOB, o_))
#define gc2deadkey(o_) (gc2tv(K_TAG_DEADKEY, o_))

/* Macro to convert a TValue into a specific heap allocated object */
#define tv2bigint(v_) ((Bigint *) gcvalue(v_))
#define tv2bigrat(v_) ((Bigrat *) gcvalue(v_))
#define tv2pair(v_) ((Pair *) gcvalue(v_))
#define tv2str(v_) ((String *) gcvalue(v_))
#define tv2sym(v_) ((Symbol *) gcvalue(v_))
#define tv2env(v_) ((Environment *) gcvalue(v_))
#define tv2cont(v_) ((Continuation *) gcvalue(v_))
#define tv2op(v_) ((Operative *) gcvalue(v_))
#define tv2app(v_) ((Applicative *) gcvalue(v_))
#define tv2enc(v_) ((Encapsulation *) gcvalue(v_))
#define tv2prom(v_) ((Promise *) gcvalue(v_))
#define tv2port(v_) ((Port *) gcvalue(v_))
#define tv2table(v_) ((Table *) gcvalue(v_))
#define tv2error(v_) ((Error *) gcvalue(v_))
#define tv2blob(v_) ((Blob *) gcvalue(v_))

#define tv2gch(v_) ((GCheader *) gcvalue(v_))
#define tv2mgch(v_) ((MGCheader *) gcvalue(v_))

/* Macro to convert any Kernel object into a GCObject */
#define obj2gco(v_) ((GCObject *) (v_))

#define obj2gch(v_) ((GCheader *) (v_))

/* Macros to access innertv values */
/* TODO: add assertions */
#define ivalue(o_) ((o_).tv.v.i)
#define bvalue(o_) ((o_).tv.v.b)
#define chvalue(o_) ((o_).tv.v.ch)
#define gcvalue(o_) ((o_).tv.v.gc)
#define pvalue(o_) ((o_).tv.v.p)
#define dvalue(o_) ((o_).d)

/* Macro to obtain a string describing the type of a TValue */#
#define ttname(tv_) (ktv_names[ttype(tv_)])

extern char *ktv_names[];

/* Macros to handle marks */
/* TODO add assertions to check that symbols aren't marked with these */

/* NOTE: this only works in markable objects, but not in symbols */
#define kget_mark(p_) (tv2mgch(p_)->mark)

#ifdef KTRACK_MARKS
/* XXX: marking macros should take a klisp_State parameter and
   keep track of marks in the klisp_State */
int32_t kmark_count;
#define kset_mark(p_, m_) ({ TValue new_mark_ = (m_); \
	TValue obj_ = (p_); \
	TValue old_mark_ = kget_mark(p_);	\
	if (kis_false(old_mark_) && !kis_false(new_mark_)) \
	    ++kmark_count; \
	else if (kis_false(new_mark_) && !kis_false(old_mark_)) \
	    --kmark_count; \
	kget_mark(obj_) = new_mark_; })
#define kcheck_mark_balance() (assert(kmark_count == 0))
#else
#define kset_mark(p_, m_) (kget_mark(p_) = (m_))
#define kcheck_mark_balance() 
#endif

/* simple boolean #t mark */
#define kmark(p_) (kset_mark(p_, KTRUE)) 
#define kunmark(p_) (kset_mark(p_, KFALSE)) 

#define kis_marked(p_) (!kis_unmarked(p_))
#define kis_unmarked(p_) (tv_equal(kget_mark(p_), KFALSE))

/* Symbols marking */
/* NOTE: it's different because symbols mark their strings */
#define kget_symbol_mark(s_) (kget_mark(tv2sym(s_)->str))
#define kset_symbol_mark(s_, m_) (kget_mark(tv2sym(s_)->str) = (m_))
#define kmark_symbol(s_) (kset_mark(tv2sym(s_)->str, KTRUE))
#define kunmark_symbol(s_) (kset_mark(tv2sym(s_)->str, KFALSE))
#define kis_symbol_marked(s_) (kis_marked(tv2sym(s_)->str))
#define kis_symbol_unmarked(s_) (kis_unmarked(tv2sym(s_)->str))

/* Macros to access kflags & type in GCHeader */
/* TODO: 1 should always be reserved for mutability flag */
#define gch_get_type(o_) (obj2gch(o_)->tt)
#define gch_get_kflags(o_) (obj2gch(o_)->kflags)
#define tv_get_kflags(o_) (gch_get_kflags(tv2gch(o_)))

/* General KFlags */
/* TODO use bittricks from kgc.h */
/* MAYBE make flags 16 bits, make gc flags 8 bits */
/* for now only used in pairs and strings */

#define K_FLAG_CAN_HAVE_NAME 0x80
#define K_FLAG_HAS_NAME 0x40

/* evaluates o_ more than once */
#define kcan_have_name(o_) \
    (iscollectable(o_) && ((tv_get_kflags(o_)) & K_FLAG_CAN_HAVE_NAME) != 0)

#define khas_name(o_) \
    (iscollectable(o_) && (tv_get_kflags(o_)) & K_FLAG_HAS_NAME)

#define K_FLAG_HAS_SI 0x20

#define kcan_have_si(o_) (iscollectable(o_))
#define khas_si(o_) ((iscollectable(o_) &&			\
		      (tv_get_kflags(o_)) & K_FLAG_HAS_SI))

#define K_FLAG_IMMUTABLE 0x10

#define kis_mutable(o_) ((tv_get_kflags(o_) & K_FLAG_IMMUTABLE) == 0)
#define kis_immutable(o_) (!kis_mutable(o_))

/* KFlags for symbols */
/* has external representation (identifiers) */
#define K_FLAG_EXT_REP 0x01
#define khas_ext_rep(s_) ((tv_get_kflags(s_) & K_FLAG_EXT_REP) != 0)

/* KFlags for marking continuations */
#define K_FLAG_OUTER 0x01
#define K_FLAG_INNER 0x02
#define K_FLAG_DYNAMIC 0x04
#define K_FLAG_BOOL_CHECK 0x08

/* evaluate c_ more than once */
#define kset_inner_cont(c_) (tv_get_kflags(c_) |= K_FLAG_INNER)
#define kset_outer_cont(c_) (tv_get_kflags(c_) |= K_FLAG_OUTER)
#define kset_dyn_cont(c_) (tv_get_kflags(c_) |= K_FLAG_DYNAMIC)
#define kset_bool_check_cont(c_) (tv_get_kflags(c_) |= K_FLAG_BOOL_CHECK)

#define kis_inner_cont(c_) ((tv_get_kflags(c_) & K_FLAG_INNER) != 0)
#define kis_outer_cont(c_) ((tv_get_kflags(c_) & K_FLAG_OUTER) != 0)
#define kis_dyn_cont(c_) ((tv_get_kflags(c_) & K_FLAG_DYNAMIC) != 0)
#define kis_bool_check_cont(c_) ((tv_get_kflags(c_) & K_FLAG_BOOL_CHECK) != 0)

#define K_FLAG_OUTPUT_PORT 0x01
#define K_FLAG_INPUT_PORT 0x02
#define K_FLAG_CLOSED_PORT 0x04

#define kport_set_input(o_) (tv_get_kflags(o_) |= K_FLAG_INPUT_PORT)
#define kport_set_output(o_) (tv_get_kflags(o_) |= K_FLAG_INPUT_PORT)
#define kport_set_closed(o_) (tv_get_kflags(o_) |= K_FLAG_CLOSED_PORT)

#define kport_is_input(o_) ((tv_get_kflags(o_) & K_FLAG_INPUT_PORT) != 0)
#define kport_is_output(o_) ((tv_get_kflags(o_) & K_FLAG_OUTPUT_PORT) != 0)
#define kport_is_closed(o_) ((tv_get_kflags(o_) & K_FLAG_CLOSED_PORT) != 0)

#define K_FLAG_WEAK_KEYS 0x01
#define K_FLAG_WEAK_VALUES 0x02
#define K_FLAG_WEAK_NOTHING 0x00

#define ktable_has_weak_keys(o_) \
    ((tv_get_kflags(o_) & K_FLAG_WEAK_KEYS) != 0)
#define ktable_has_weak_values(o_) \
    ((tv_get_kflags(o_) & K_FLAG_WEAK_VALUES) != 0)

/* can't be inline because we also use pointers to them,
 (at least gcc doesn't bother to create them and the linker fails) */
bool kis_input_port(TValue o);
bool kis_output_port(TValue o);

/* Macro to test the most basic equality on TValues */
#define tv_equal(tv1_, tv2_) ((tv1_).raw == (tv2_).raw)

/* Symbols could be eq? but not tv_equal? because of source info */
#define tv_sym_equal(sym1_, sym2_) \
    (tv_equal(tv2sym(sym1_)->str, tv2sym(sym2_)->str))

/*
** for internal debug only
*/
#define checkconsistency(obj) \
  klisp_assert(!iscollectable(obj) || (ttype_(obj) == gcvalue(obj)->gch.tt))

#define checkliveness(k,obj) \
  klisp_assert(!iscollectable(obj) || \
  ((ttype_(obj) == gcvalue(obj)->gch.tt) && !isdead(k, gcvalue(obj))))


#endif
