/*
** kgstrings.c
** Strings features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
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
#include "ksymbol.h"
#include "kstring.h"

#include "kghelpers.h"
#include "kgstrings.h"

/* 13.1.1? string? */
/* uses typep */

/* 13.1.2? make-string */
void make_string(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_al1tp(K, "make-string", ptree, "finite number", ttisfixint, tv_s, 
	       maybe_char);

    char fill = ' ';
    if (get_opt_tpar(K, "make-string", K_TCHAR, &maybe_char))
	fill = chvalue(maybe_char);

    if (ivalue(tv_s) < 0) {
	klispE_throw(K, "make-string: negative size");    
	return;
    }

    TValue new_str = kstring_new_sc(K, ivalue(tv_s), fill);
    kapply_cc(K, new_str);
}

/* 13.1.3? string-length */
void string_length(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, "string-length", ptree, "string", ttisstring, str);

    TValue res = i2tv(kstring_size(str));
    kapply_cc(K, res);
}

/* 13.1.4? string-ref */
void string_ref(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, "string-ref", ptree, "string", ttisstring, str,
	     "finite integer", ttisfixint, tv_i);

    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kstring_size(str)) {
	/* TODO show index */
	klispE_throw(K, "string-ref: index out of bounds");
	return;
    }

    TValue res = ch2tv(kstring_buf(str)[i]);
    kapply_cc(K, res);
}

/* 13.1.5? string-set! */
void string_setS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, "string-set!", ptree, "string", ttisstring, str,
	     "finite integer", ttisfixint, tv_i, "char", ttischar, tv_ch);

    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kstring_size(str)) {
	/* TODO show index */
	klispE_throw(K, "string-set!: index out of bounds");
	return;
    }

    kstring_buf(str)[i] = chvalue(tv_ch);
    kapply_cc(K, KINERT);
}

/* 13.2.1? string */
/* TODO */

/* 13.2.2? string=?, string-ci=? */
/* TODO */

/* 13.2.3? string<?, string<=?, string>?, string>=? */
/* TODO */

/* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
/* TODO */

/* 13.2.5? substring */
/* TODO */

/* 13.2.6? string-append */
/* TODO */

/* 13.2.7? string->list, list->string */
/* TODO */

/* 13.2.8? string-copy */
/* TODO */

/* 13.2.9? string-fill! */
void string_fillS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, "string-fill!", ptree, "string", ttisstring, str,
	     "char", ttischar, tv_ch);

    memset(kstring_buf(str), chvalue(tv_ch), kstring_size(str));
    kapply_cc(K, KINERT);
}


/* 13.3.1? symbol->string */
/* TEMP: for now all strings are mutable, this returns a new object
   each time */
/* TODO */

/* 13.3.2? symbol->string */
/* TEMP: for now this can create symbols with no external representation
   this includes all symbols with non identifiers characters.
*/
/* NOTE:
   Symbols with uppercase alphabetic characters will write as lowercase and
   so, when read again will not compare as either eq? or equal?. This is ok
   because the report only says that read objects when written and read 
   again must be equal? which happens here 
*/
/* TODO */
