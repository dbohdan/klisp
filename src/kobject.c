/*
** kobject.c
** Type definitions for Kernel Objects
** See Copyright Notice in klisp.h
*/

#include "kobject.h"

#ifdef KTRACK_MARKS
int32_t kmark_count = 0;
#endif
/*
** The global const variables
*/
const TValue knil = KNIL_;
const TValue kignore = KIGNORE_;
const TValue kinert = KINERT_;
const TValue keof = KEOF_;
const TValue ktrue = KTRUE_;
const TValue kfalse = KFALSE_;
const TValue kepinf = KEPINF_;
const TValue keminf = KEMINF_;
const TValue kipinf = KIPINF_;
const TValue kiminf = KIMINF_;
const TValue krwnpv = KRWNPV_;
const TValue kundef = KUNDEF_;
const TValue kfree = KFREE_;

const TValue knull = KNULL_;
const TValue kalarm = KALARM_;
const TValue kbackspace = KBACKSPACE_;
const TValue ktab = KTAB_;
const TValue knewline = KNEWLINE_;
const TValue kreturn = KRETURN_;
const TValue kescape = KESCAPE_;
const TValue kspace = KSPACE_;
const TValue kdelete = KDELETE_;
const TValue kvtab = KVTAB_;
const TValue kformfeed = KFORMFEED_;

/*
** The name strings for all TValue types
** This should be updated if types are modified in kobject.h
*/
char *ktv_names[] = {
    [K_TFIXINT] = "fixint",
    [K_TBIGINT] = "bigint", 
    [K_TFIXRAT] = "fixrat", 
    [K_TBIGRAT] = "bigrat", 
    [K_TEINF] = "einf", 
    [K_TDOUBLE] = "double", 
    [K_TBDOUBLE] = "bdouble", 
    [K_TIINF] = "einf", 
    [K_TIINF] = "iinf", 
    
    [K_TRWNPV] = "rwnpv", 
    [K_TCOMPLEX] = "complex", 
    [K_TUNDEFINED] = "undefined", 

    [K_TNIL] = "nil",          
    [K_TIGNORE] = "ignore",
    [K_TINERT] = "inert", 
    [K_TEOF] = "eof", 
    [K_TBOOLEAN] = "boolean", 
    [K_TCHAR] = "char", 
    [K_TFREE] = "free entry", 
    [K_TDEADKEY] = "dead key", 

    [K_TUSER] = "user pointer", 

    [K_TPAIR] = "pair", 
    [K_TSTRING] = "string", 
    [K_TSYMBOL] = "symbol",
    [K_TENVIRONMENT] = "environment",
    [K_TCONTINUATION] = "continuation",
    [K_TOPERATIVE] = "operative",
    [K_TAPPLICATIVE] = "applicative",
    [K_TENCAPSULATION] = "encapsulation",
    [K_TPROMISE] = "promise",
    [K_TTABLE] = "table", 
    [K_TERROR] = "error", 
    [K_TBYTEVECTOR] = "bytevector",
    [K_TFPORT] = "file port",
    [K_TMPORT] = "mem port",
    [K_TKEYWORD] = "keyword",
    [K_TLIBRARY] = "library"
};

int32_t klispO_log2 (uint32_t x) {
    static const uint8_t log_2[256] = {
        0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
        8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8
    };
    int32_t l = -1;
    while (x >= 256) { l += 8; x >>= 8; }
    return l + log_2[x];
}
