/*
** kgkeywords.c
** Keyword features for the ground environment
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
#include "kstring.h"
#include "ksymbol.h"
#include "kkeyword.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgkeywords.h"

/* ?.? keyword? */
/* uses typep */

/* ?.? keyword->string, string->keyword */
void keyword_to_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "keyword", ttiskeyword, keyw);
    /* The strings in keywords are immutable so we can just return that */
    TValue str = kkeyword_str(keyw);
    kapply_cc(K, str);
}

void string_to_keyword(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);
    /* If the string is mutable it is copied */
    TValue new_keyw = kkeyword_new_str(K, str);
    kapply_cc(K, new_keyw);
}

/* ?.? keyword->symbol, string->symbol */
void keyword_to_symbol(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "keyword", ttiskeyword, keyw);
    TValue sym = ksymbol_new_str(K, kkeyword_str(keyw), KNIL);
    kapply_cc(K, sym);
}

void symbol_to_keyword(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "symbol", ttissymbol, sym);
    TValue new_keyw = kkeyword_new_str(K, ksymbol_str(sym));
    kapply_cc(K, new_keyw);
}

/* init ground */
void kinit_keywords_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /*
    ** This section is missing from the report. The bindings here are
    ** should not be considered standard. 
    */

    /* ?.? keyword? */
    add_applicative(K, ground_env, "keyword?", typep, 2, symbol, 
                    i2tv(K_TKEYWORD));
    /* ?.? keyword->string, string->keyword */
    add_applicative(K, ground_env, "keyword->string", keyword_to_string, 0);
    add_applicative(K, ground_env, "string->keyword", string_to_keyword, 0);
    /* ?.? keyword->symbol, symbol->keyword */
    add_applicative(K, ground_env, "keyword->symbol", keyword_to_symbol, 0);
    add_applicative(K, ground_env, "symbol->keyword", symbol_to_keyword, 0);
}
