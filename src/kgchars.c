/*
** kgchars.c
** Characters features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>

#include "kstate.h"
#include "kobject.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kchar.h"

#include "kghelpers.h"
#include "kgchars.h"

/* 14.1.1? char? */
/* uses typep */

/* 14.1.2? char-alphabetic?, char-numeric?, char-whitespace? */
/* use ftyped_predp */

/* 14.1.3? char-upper-case?, char-lower-case? */
/* use ftyped_predp */

/* 14.1.4? char->integer, integer->char */
void kchar_to_integer(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "character", ttischar, ch);

    kapply_cc(K, i2tv((int32_t) chvalue(ch)));
}

void kinteger_to_char(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "exact integer", ttiseinteger, itv);
    
    if (ttisbigint(itv)) {
	klispE_throw_simple(K, "integer out of ASCII range [0 - 127]");
	return;
    }
    int32_t i = ivalue(itv);

    /* for now only allow ASCII */
    if (i < 0 || i > 127) {
	klispE_throw_simple(K, "integer out of ASCII range [0 - 127]");
	return;
    }
    kapply_cc(K, ch2tv((char) i));
}

/* 14.1.4? char-upcase, char-downcase, char-titlecase, char-foldcase */
void kchar_change_case(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: conversion fn
    */
    UNUSED(denv);
    bind_1tp(K, ptree, "character", ttischar, chtv);
    char ch = chvalue(chtv);
    char (*fn)(char) = pvalue(xparams[0]);
    ch = fn(ch);
    kapply_cc(K, ch2tv(ch));
}

/* 14.2.1? char=? */
/* uses ftyped_bpredp */

/* 14.2.2? char<?, char<=?, char>?, char>=? */
/* use ftyped_bpredp */

/* 14.2.3? char-ci=? */
/* uses ftyped_bpredp */

/* 14.2.4? char-ci<?, char-ci<=?, char-ci>?, char-ci>=? */
/* use ftyped_bpredp */

/* 14.2.? char-digit?, char->digit, digit->char */
void char_digitp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(denv);
    UNUSED(xparams);
    bind_al1tp(K, ptree, "character", ttischar, chtv, basetv);

    int base = 10; /* default */

    if (get_opt_tpar(K, basetv, "base [2-36]", ttisbase)) {
	base = ivalue(basetv); 
    }
    char ch = tolower(chvalue(chtv));
    bool b = (isdigit(ch) && (ch - '0') < base) || 
	(isalpha(ch) && (ch - 'a' + 10) < base);
    kapply_cc(K, b2tv(b));
}

void char_to_digit(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(denv);
    UNUSED(xparams);
    bind_al1tp(K, ptree, "character", ttischar, chtv, basetv);

    int base = 10; /* default */

    if (get_opt_tpar(K, basetv, "base [2-36]", ttisbase)) {
	base = ivalue(basetv); 
    }
    char ch = tolower(chvalue(chtv));
    int digit = 0;

    if (isdigit(ch) && (ch - '0') < base)
	digit = ch - '0';
    else if (isalpha(ch) && (ch - 'a' + 10) < base)
	digit = ch - 'a' + 10;
    else {
	klispE_throw_simple_with_irritants(K, "Not a digit in this base", 
					  2, ch2tv(ch), i2tv(base));
	return;
    }
    kapply_cc(K, i2tv(digit));
}

void digit_to_char(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));

    UNUSED(denv);
    UNUSED(xparams);
    bind_al1tp(K, ptree, "exact integer", ttiseinteger, digittv, basetv);

    int base = 10; /* default */

    if (get_opt_tpar(K, basetv, "base [2-36]", ttisbase)) {
	base = ivalue(basetv); 
    }

    if (ttisbigint(digittv) || ivalue(digittv) < 0 || 
            ivalue(digittv) >= base) {
	klispE_throw_simple_with_irritants(K, "Not a digit in this base",
					   2, digittv, i2tv(base));
	return;
    }
    int digit = ivalue(digittv);
    char ch = digit <= 9? 
	'0' + digit : 
	'a' + (digit - 10);
    kapply_cc(K, ch2tv(ch));
}

/* init ground */
void kinit_chars_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from r5rs scheme and should not be considered standard. They are
    ** provided in the meantime to allow programs to use character features
    ** (ASCII only). 
    */

    /* 14.1.1? char? */
    add_applicative(K, ground_env, "char?", typep, 2, symbol, 
		    i2tv(K_TCHAR));
    /* 14.1.2? char-alphabetic?, char-numeric?, char-whitespace? */
    /* unlike in r5rs these take an arbitrary number of chars
       (even cyclical list) */
    add_applicative(K, ground_env, "char-alphabetic?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_alphabeticp));
    add_applicative(K, ground_env, "char-numeric?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_numericp));
    add_applicative(K, ground_env, "char-whitespace?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_whitespacep));
    /* 14.1.3? char-upper-case?, char-lower-case? */
    /* unlike in r5rs these take an arbitrary number of chars
       (even cyclical list) */
    add_applicative(K, ground_env, "char-upper-case?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_upper_casep));
    add_applicative(K, ground_env, "char-lower-case?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_lower_casep));
    /* 14.1.4? char->integer, integer->char */
    add_applicative(K, ground_env, "char->integer", kchar_to_integer, 0);
    add_applicative(K, ground_env, "integer->char", kinteger_to_char, 0);
    /* 14.1.4? char-upcase, char-downcase, char-titlecase, char-foldcase */
    add_applicative(K, ground_env, "char-upcase", kchar_change_case, 1,
		    p2tv(toupper));
    add_applicative(K, ground_env, "char-downcase", kchar_change_case, 1,
		    p2tv(tolower));
    add_applicative(K, ground_env, "char-titlecase", kchar_change_case, 1,
		    p2tv(toupper));
    add_applicative(K, ground_env, "char-foldcase", kchar_change_case, 1,
		    p2tv(tolower));
    /* 14.2.1? char=? */
    add_applicative(K, ground_env, "char=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_eqp));
    /* 14.2.2? char<?, char<=?, char>?, char>=? */
    add_applicative(K, ground_env, "char<?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ltp));
    add_applicative(K, ground_env, "char<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp),  p2tv(kchar_lep));
    add_applicative(K, ground_env, "char>?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_gtp));
    add_applicative(K, ground_env, "char>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_gep));
    /* 14.2.3? char-ci=? */
    add_applicative(K, ground_env, "char-ci=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_eqp));
    /* 14.2.4? char-ci<?, char-ci<=?, char-ci>?, char-ci>=? */
    add_applicative(K, ground_env, "char-ci<?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_ltp));
    add_applicative(K, ground_env, "char-ci<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp),  p2tv(kchar_ci_lep));
    add_applicative(K, ground_env, "char-ci>?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_gtp));
    add_applicative(K, ground_env, "char-ci>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_gep));
    /* 14.2.? char-digit?, char->digit, digit->char */
    add_applicative(K, ground_env, "char-digit?", char_digitp, 0);
    add_applicative(K, ground_env, "char->digit", char_to_digit, 0);
    add_applicative(K, ground_env, "digit->char", digit_to_char, 0);
}
