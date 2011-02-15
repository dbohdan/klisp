/*
** kobject.h
** Type definitions for Kernel Objects
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: While the tagging system comes from Mozilla TraceMonkey,
** o code from TraceMonkey was used.
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
*/

/*
** Tagged values in 64 bits (for 32 bit systems)
** NaN boxing: Values are encoded as double precision NaNs
** There is one canonical NaN that is used through the interpreter
** and all remaining NaNs are used to encode the rest of the types
** (other than double)
** Canonical NaN: (0)(111 1111 1111) 1000  0000 0000 0000 0000 32(0)
** Infinities: s(111 1111 1111) 0000  0000 0000 0000 0000 32(0)
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

#define K_TNIL 20
#define K_TIGNORE 21
#define K_TINERT 22
#define K_TEOF 23
#define K_TBOOLEAN 24
#define K_TCHAR 25

#define K_TPAIR        30
#define K_TSTRING	31
#define K_TSYMBOL	32

#define K_MAKE_VTAG(t) (K_TAG_TAGGED | t)

/* TODO: For now we will only use fixints */
#define K_TAG_FIXINT	K_MAKE_VTAG(K_TFIXINT)

#define K_TAG_NIL	K_MAKE_VTAG(K_TNIL)
#define K_TAG_IGNORE	K_MAKE_VTAG(K_TIGNORE)
#define K_TAG_INERT	K_MAKE_VTAG(K_TINERT)
#define K_TAG_EOF	K_MAKE_VTAG(K_TEOF)
#define K_TAG_BOOLEAN	K_MAKE_VTAG(K_TBOOLEAN)
#define K_TAG_CHAR	K_MAKE_VTAG(K_TCHAR)

#define K_TAG_PAIR K_MAKE_VTAG(K_TPAIR)
#define K_TAG_STRING K_MAKE_VTAG(K_TSTRING)
#define K_TAG_SYMBOL K_MAKE_VTAG(K_TSYMBOL)

/*
** Macros to test types
*/

/*
** This is intended for use in switch statements
** TODO: decide if inexact infinities and reals with no
** primary values are included in K_TDOUBLE
*/
#define ttype(o) ({ TValue o_ = o;			\
	    ttisdouble(o_)? K_TDOUBLE : ttype_(o_); })

/*
** This is intended for internal use below. DON'T USE OUTSIDE THIS FILE
*/
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
    unsigned char ch;
    GCObject *gc; 
    void *p; 
    /* ... */
} Value;

/*
** All Kernel non heap-allocated values (except doubles) tagged
*/
typedef struct __attribute__ ((__packed__)) InnerTV {
    uint32_t t;
    Value v;
} InnerTV;

/*
** Union of all Kernel non heap-allocated values
*/
typedef __attribute__((aligned (8))) union {
    double d;
    InnerTV tv;
} TValue;

/*
** Individual heap-allocated values
*/
typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    TValue car;
    TValue cdr;
} Pair;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    unsigned char b[]; // buffer
} Symbol;

typedef struct __attribute__ ((__packed__)) {
    CommonHeader;
    uint32_t size; // to allow embedded '\0'
    unsigned char b[]; // buffer
} String;

/*
** Union of all Kernel heap-allocated values
*/

/* LUA NOTE: In Lua the corresponding union is in lstate.h */
union GCObject {
    GCheader gch;
    Pair pair;
    Symbol sym;
    String str;
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

#define KNIL ((TValue) KNIL_)
#define KINERT ((TValue) KINERT_)
#define KIGNORE ((TValue) KIGNORE_)
#define KEOF ((TValue) KEOF_)
#define KTRUE ((TValue) KTRUE_)
#define KFALSE ((TValue) KFALSE_)

/*
** The same constants as global const variables
*/
const TValue knil = KNIL_;
const TValue kignore = KIGNORE_;
const TValue kinert = KINERT_;
const TValue keof = KEOF_;
const TValue ktrue = KTRUE_;
const TValue kfalse = KFALSE_;

#endif
