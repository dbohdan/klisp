/*
** kgstrings.c
** Strings features for the ground environment
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
#include "ksymbol.h"
#include "kstring.h"

#include "kghelpers.h"
#include "kgstrings.h"

/* 13.1.1? string? */
/* uses typep */

/* 13.1.2? make-string */
void kgmake_string(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
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
void kgstring_length(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, "string-length", ptree, "string", ttisstring, str);

    TValue res = i2tv(kstring_size(str));
    kapply_cc(K, res);
}

/* 13.1.4? string-ref */
/* TODO */

/* 13.1.5? string-set! */
/* TODO */

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
/* TODO */

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
