/*
** kgstrings.h
** Strings features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgstrings_h
#define kgstrings_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 13.1.1? string? */
/* uses typep */

/* 13.1.? immutable-string?, mutable-string? */
/* use ftypep */

/* 13.1.2? make-string */
void make_string(klisp_State *K);

/* 13.1.3? string-length */
void string_length(klisp_State *K);

/* 13.1.4? string-ref */
void string_ref (klisp_State *K);

/* 13.1.5? string-set! */
void string_setS (klisp_State *K);

/* 13.2.1? string */
void string(klisp_State *K);

/* 13.2.2? string=?, string-ci=? */
/* use ftyped_bpredp */

/* 13.2.3? string<?, string<=?, string>?, string>=? */
/* use ftyped_bpredp */

/* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
/* use ftyped_bpredp */

/* Helpers for binary predicates */
/* XXX: this should probably be in file kstring.h */
bool kstring_eqp(TValue str1, TValue str2);
bool kstring_ci_eqp(TValue str1, TValue str2);

bool kstring_ltp(TValue str1, TValue str2);
bool kstring_lep(TValue str1, TValue str2);
bool kstring_gtp(TValue str1, TValue str2);
bool kstring_gep(TValue str1, TValue str2);

bool kstring_ci_ltp(TValue str1, TValue str2);
bool kstring_ci_lep(TValue str1, TValue str2);
bool kstring_ci_gtp(TValue str1, TValue str2);
bool kstring_ci_gep(TValue str1, TValue str2);


/* 13.2.5? substring */
void substring(klisp_State *K);

/* 13.2.6? string-append */
void string_append(klisp_State *K);

/* 13.2.7? string->list, list->string */
void list_to_string(klisp_State *K);
void string_to_list(klisp_State *K);

/* 13.2.8? string-copy */
void string_copy(klisp_State *K);

/* 13.2.9? string->immutable-string */
void string_to_immutable_string(klisp_State *K);

/* 13.2.10? string-fill! */
void string_fillS(klisp_State *K);

/* Helpers */
bool kstringp(TValue obj);

/* init ground */
void kinit_strings_ground_env(klisp_State *K);

#endif
