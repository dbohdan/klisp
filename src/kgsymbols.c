/*
** kgsymbols.c
** Symbol features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kcontinuation.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgsymbols.h"

/* 4.4.1 symbol? */
/* uses typep */

/* 13.3.1? symbol->string */
/* The strings in symbols are immutable so we can just return that */
void symbol_to_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "symbol", ttissymbol, sym);
    TValue str = ksymbol_str(sym);
    kapply_cc(K, str);
}

/* 13.3.2? string->symbol */
void string_to_symbol(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);
    /* TODO si */
    /* If the string is mutable it is copied */
    TValue new_sym = ksymbol_new_str(K, str, KNIL);
    kapply_cc(K, new_sym);
}

/* init ground */
void kinit_symbols_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* 4.4.1 symbol? */
    add_applicative(K, ground_env, "symbol?", typep, 2, symbol, 
                    i2tv(K_TSYMBOL));
    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from r5rs scheme and should not be considered standard. 
    */
    /* ?.?.1? symbol->string */
    add_applicative(K, ground_env, "symbol->string", symbol_to_string, 0);
    /* ?.?.2? string->symbol */
    add_applicative(K, ground_env, "string->symbol", string_to_symbol, 0);
}
