/*
** kobject.h
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
const TValue kspace = KSPACE_;
const TValue knewline = KNEWLINE_;

/*
** The name strings for all TValue types
*/
char *ktv_names[] = {
    [K_TFIXINT] = "fixint",
    [K_TBIGINT] = "bigint", 
    [K_TFIXRAT] = "fixrat", 
    [K_TBIGRAT] = "bigrat", 
    [K_TEINF] = "einf", 
    [K_TDOUBLE] = "double", 
    [K_TBDOUBLE] = "bdouble", 
    [K_TIINF] = "iinf", 
    [K_TRWNPN] = "rwnpn", 
    [K_TCOMPLEX] = "complex", 

    [K_TNIL] = "nil",       
    [K_TIGNORE] = "ignore",
    [K_TINERT] = "inert", 
    [K_TEOF] = "eof", 
    [K_TBOOLEAN] = "boolean", 
    [K_TCHAR] = "char", 

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
    [K_TPORT] = "port"
};

bool kis_input_port(TValue o)
{
    return ttisport(o) && kport_is_input(o);
}

bool kis_output_port(TValue o)
{
    return ttisport(o) && kport_is_output(o);
}

