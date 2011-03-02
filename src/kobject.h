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

/*
** Union of all collectible objects
*/
typedef union GCObject GCObject;

/*
** Common Header for all collectible objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader GCObject *next; uint16_t tt; uint16_t gct; 


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
#define K_TEINF         4
#define K_TDOUBLE       5
#define K_TBDOUBLE      6
#define K_TIINF         7
#define K_TRWNPN        8
#define K_TCOMPLEX      9

#define K_TNIL 		20
#define K_TIGNORE 	21
#define K_TINERT 	22
#define K_TEOF 		23
#define K_TBOOLEAN 	24
#define K_TCHAR 	25

#define K_TPAIR        	30
#define K_TSTRING	31
#define K_TSYMBOL	32
#define K_TENVIRONMENT  33
#define K_TCONTINUATION 34
#define K_TOPERATIVE    35
#define K_TAPPLICATIVE  36

#define K_MAKE_VTAG(t) (K_TAG_TAGGED | (t))

/*
** TODO: 
**
** - decide if inexact infinities and reals with no
**    primary values are included in K_TDOUBLE
** - For now we will only use fixints and exact infinities 
*/
#define K_TAG_FIXINT	K_MAKE_VTAG(K_TFIXINT)
#define K_TAG_EINF	K_MAKE_VTAG(K_TEINF)

#define K_TAG_NIL	K_MAKE_VTAG(K_TNIL)
#define K_TAG_IGNORE	K_MAKE_VTAG(K_TIGNORE)
#define K_TAG_INERT	K_MAKE_VTAG(K_TINERT)
#define K_TAG_EOF	K_MAKE_VTAG(K_TEOF)
#define K_TAG_BOOLEAN	K_MAKE_VTAG(K_TBOOLEAN)
#define K_TAG_CHAR	K_MAKE_VTAG(K_TCHAR)

#define K_TAG_PAIR K_MAKE_VTAG(K_TPAIR)
#define K_TAG_STRING K_MAKE_VTAG(K_TSTRING)
#define K_TAG_SYMBOL K_MAKE_VTAG(K_TSYMBOL)

#define K_TAG_SYMBOL K_MAKE_VTAG(K_TSYMBOL)
#define K_TAG_ENVIRONMENT K_MAKE_VTAG(K_TENVIRONMENT)
#define K_TAG_CONTINUATION K_MAKE_VTAG(K_TCONTINUATION)
#define K_TAG_OPERATIVE K_MAKE_VTAG(K_TOPERATIVE)
#define K_TAG_APPLICATIVE K_MAKE_VTAG(K_TAPPLICATIVE)


/*
** Macros to test types
*/

/* NOTE: This is intended for use in switch statements */
#define ttype(o) ({ TValue o_ = (o);			\
	    ttisdouble(o_)? K_TDOUBLE : ttype_(o_); })

/* This is intended for internal use below. DON'T USE OUTSIDE THIS FILE */
#define ttag(o) ((o).tv.t)
#define ttype_(o) (K_TAG_TYPE(ttag(o)))
#define tflag_(o) (K_TAG_FLAG(ttag(o)))
#define tbasetype_(o) (K_TAG_BASE_TYPE(ttag(o)))

/* Simple types (value in TValue struct) */
#define ttisfixint(o)	(tbasetype_(o) == K_TAG_FIXINT)
#define ttisnil(o)	(tbasetype_(o) == K_TAG_NIL)
#define ttisignore(o)	(tbasetype_(o) == K_TAG_IGNORE)
#define ttisinert(o)	(tbasetype_(o) == K_TAG_INERT)
#define ttiseof(o)	(tbasetype_(o) == K_TAG_EOF)
#define ttisboolean(o)	(tbasetype_(o) == K_TAG_BOOLEAN)
#define ttischar(o)	(tbasetype_(o) == K_TAG_CHAR)
#define ttisdouble(o)	((ttag(o) & K_TAG_BASE_MASK) != K_TAG_TAGGED) 

/* Complex types (value in heap) */
#define ttisstring(o)	(tbasetype_(o) == K_TAG_STRING)
#define ttissymbol(o)	(tbasetype_(o) == K_TAG_SYMBOL)
#define ttispair(o)	(tbasetype_(o) == K_TAG_PAIR)


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
    TValue mark; /* for cycle/sharing aware algorithms */
    TValue car;
    TValue cdr;
    TValue si; /* source code info (either () or (filename line col) */
} Pair;

/* XXX: Symbol should probably contain a String instead of a char buf */
typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for cycle/sharing aware algorithms */
    uint32_t size;
    char b[];
} Symbol;


typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue mark; /* for cycle/sharing aware algorithms */
    TValue parents; /* may be (), a list, or a single env */
    TValue bindings; /* TEMP: for now alist of (binding . value) */
} Environment;

/*
** prototype for callable c functions from the interpreter main loop:
**
** TEMP: Has to be here because it uses TValue type
** ideally it should be in klisp.h
*/
typedef void (*klisp_Ifunc) (TValue *ud, TValue val, TValue env);

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue name; /* cont name/type */
    TValue si; /* source code info (either () or (filename line col) */
    klisp_Ifunc fn; /* the function that does the work */
    int32_t extra_size;
    TValue extra[];
} Continuation;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue name;
    TValue si; /* source code info (either () or (filename line col) */
    klisp_Ifunc fn; /* the function that does the work */
    int32_t extra_size;
    TValue extra[];
} Operative;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue name; 
    TValue si; /* source code info (either () or (filename line col) */
    TValue underlying; /* underlying operative/applicative */
} Applicative;


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
    char b[]; // buffer
} String;

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

/* RATIONALE: the ones above can be used in initializers */
#define KNIL ((TValue) KNIL_)
#define KINERT ((TValue) KINERT_)
#define KIGNORE ((TValue) KIGNORE_)
#define KEOF ((TValue) KEOF_)
#define KTRUE ((TValue) KTRUE_)
#define KFALSE ((TValue) KFALSE_)
#define KEPINF ((TValue) KEPINF_)
#define KEMINF ((TValue) KEMINF_)

/* The same constants as global const variables */
const TValue knil;
const TValue kignore;
const TValue kinert;
const TValue keof;
const TValue ktrue;
const TValue kfalse;
const TValue kepinf;
const TValue keminf;

/* Macros to create TValues of non-heap allocated types (for initializers) */
#define ch2tv_(ch_) {.tv = {.t = K_TAG_CHAR, .v = { .ch = (ch_) }}}
#define i2tv_(i_) {.tv = {.t = K_TAG_FIXINT, .v = { .i = (i_) }}}
#define b2tv_(b_) {.tv = {.t = K_TAG_BOOLEAN, .v = { .b = (b_) }}}

/* Macros to create TValues of non-heap allocated types */
#define ch2tv(ch_) ((TValue) ch2tv_(ch_))
#define i2tv(i_) ((TValue) i2tv_(i_))
#define b2tv(b_) ((TValue) b2tv_(b_))

/* Macros to convert a GCObject * into a tagged value */
/* TODO: add assertions */
/* LUA NOTE: the corresponding defines are in lstate.h */
#define gc2tv(t_, o_) ((TValue) {.tv = {.t = (t_),			\
					.v = { .gc = obj2gco(o_)}}})
#define gc2pair(o_) (gc2tv(K_TAG_PAIR, o_))
#define gc2str(o_) (gc2tv(K_TAG_STRING, o_))
#define gc2sym(o_) (gc2tv(K_TAG_SYMBOL, o_))
#define gc2env(o_) (gc2tv(K_TAG_ENVIRONMENT, o_))

/* Macro to convert a TValue into a specific heap allocated object */
#define tv2pair(v_) ((Pair *) gcvalue(v_))
#define tv2str(v_) ((String *) gcvalue(v_))
#define tv2sym(v_) ((Symbol *) gcvalue(v_))
#define tv2env(v_) ((Environment *) gcvalue(v_))

#define tv2mgch(v_) ((MGCheader *) gcvalue(v_))

/* Macro to convert any Kernel object into a GCObject */
#define obj2gco(v_) ((GCObject *) (v_))

/* Macros to access innertv values */
/* TODO: add assertions */
#define ivalue(o_) ((o_).tv.v.i)
#define bvalue(o_) ((o_).tv.v.b)
#define chvalue(o_) ((o_).tv.v.ch)
#define gcvalue(o_) ((o_).tv.v.gc)

/* Macro to obtain a string describing the type of a TValue */#
#define ttname(tv_) (ktv_names[ttype(tv_)])

extern char *ktv_names[];

/* Macros to handle marks */
/* NOTE: this only works in markable objects */
#define kget_mark(p_) (tv2mgch(p_)->mark) 
#define kset_mark(p_, m_) (kget_mark(p_) = (m_))
/* simple boolean #t mark */
#define kmark(p_) (kset_mark(p_, KTRUE)) 
#define kunmark(p_) (kset_mark(p_, KFALSE)) 
#define kis_marked(p_) (!kis_unmarked(p_))
#define kis_unmarked(p_) (tv_equal(kget_mark(p_), KFALSE))

/* Macro to test the most basic equality on TValues */
#define tv_equal(tv1_, tv2_) ((tv1_).raw == (tv2_).raw)

#endif
