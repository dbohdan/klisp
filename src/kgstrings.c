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
#include "kchar.h"
#include "kstring.h"
#include "kvector.h"
#include "kbytevector.h"

#include "kghelpers.h"
#include "kgstrings.h"

/* 13.1.1? string? */
/* uses typep */

/* 13.1.? immutable-string?, mutable-string? */
/* use ftypep */

/* 13.1.2? make-string */
void make_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_al1tp(K, ptree, "exact integer", keintegerp, tv_s, 
               maybe_char);

    char fill = ' ';
    if (get_opt_tpar(K, maybe_char, "char", ttischar))
        fill = chvalue(maybe_char);

    if (knegativep(tv_s)) {
        klispE_throw_simple(K, "negative size");    
        return;
    } else if (!ttisfixint(tv_s)) {
        klispE_throw_simple(K, "size is too big");    
        return;
    }

    TValue new_str = kstring_new_sf(K, ivalue(tv_s), fill);
    kapply_cc(K, new_str);
}

/* 13.1.3? string-length */
void string_length(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);

    TValue res = i2tv(kstring_size(str));
    kapply_cc(K, res);
}

/* 13.1.4? string-ref */
void string_ref(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "string", ttisstring, str,
             "exact integer", keintegerp, tv_i);

    if (!ttisfixint(tv_i)) {
        /* TODO show index */
        klispE_throw_simple(K, "index out of bounds");
        return;
    }
    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kstring_size(str)) {
        /* TODO show index */
        klispE_throw_simple(K, "index out of bounds");
        return;
    }

    TValue res = ch2tv(kstring_buf(str)[i]);
    kapply_cc(K, res);
}

/* 13.1.5? string-set! */
void string_setB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, ptree, "string", ttisstring, str,
             "exact integer", keintegerp, tv_i, "char", ttischar, tv_ch);

    if (!ttisfixint(tv_i)) {
        /* TODO show index */
        klispE_throw_simple(K, "index out of bounds");
        return;
    } else if (kstring_immutablep(str)) {
        klispE_throw_simple(K, "immutable string");
        return;
    }

    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kstring_size(str)) {
        /* TODO show index */
        klispE_throw_simple(K, "index out of bounds");
        return;
    }

    kstring_buf(str)[i] = chvalue(tv_ch);
    kapply_cc(K, KINERT);
}

/* 13.2.1? string */
void string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    /* don't allow cycles */
    int32_t pairs;
    check_typed_list(K, kcharp, false, ptree, &pairs, NULL);
    TValue new_str = list_to_string_h(K, ptree, pairs);
    kapply_cc(K, new_str);
}

/* 13.?? string-upcase, string-downcase, string-titlecase, string-foldcase */
/* this will work for upcase, downcase and foldcase (in ASCII) */
void kstring_change_case(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: conversion fn
    */
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);
    char (*fn)(char) = pvalue(xparams[0]);
    int32_t size = kstring_size(str);
    TValue res = kstring_new_bs(K, kstring_buf(str), size);
    char *buf = kstring_buf(res);
    for(int32_t i = 0; i < size; ++i, buf++) {
        *buf = fn(*buf);
    }
    kapply_cc(K, res);
}

void kstring_title_case(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);
    uint32_t size = kstring_size(str);
    TValue res = kstring_new_bs(K, kstring_buf(str), size);
    char *buf = kstring_buf(res);
    bool first = true;
    while(size-- > 0) {
        char ch = *buf;
        if (ch == ' ')
            first = true;
        else if (!first)
            *buf = tolower(ch);
        else if (isalpha(ch)) { 
            /* only count as first letter something that can be capitalized */
            *buf = toupper(ch);
            first = false;
        } 
        ++buf;
    }
    kapply_cc(K, res);
}

/* 13.2.2? string=?, string-ci=? */
/* use ftyped_bpredp */

/* 13.2.3? string<?, string<=?, string>?, string>=? */
/* use ftyped_bpredp */

/* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
/* use ftyped_bpredp */

/* Helpers for binary predicates */
/* XXX: this should probably be in file kstring.h */

bool kstring_eqp(TValue str1, TValue str2) { 
    return tv_equal(str1, str2) || kstring_equalp(str1, str2);
}

bool kstring_ci_eqp(TValue str1, TValue str2)
{
    int32_t size = kstring_size(str1);
    if (kstring_size(str2) != size)
        return false;
    else {
        char *buf1 = kstring_buf(str1);
        char *buf2 = kstring_buf(str2);

        while(size--) {
            if (tolower(*buf1) != tolower(*buf2))
                return false;
            buf1++, buf2++;
        }
        return true;
    }
}

bool kstring_ltp(TValue str1, TValue str2)
{
    int32_t size1 = kstring_size(str1);
    int32_t size2 = kstring_size(str2);

    int32_t min_size = size1 < size2? size1 : size2;
    /* memcmp > 0 if str1 has a bigger char in first diff position */
    int res = memcmp(kstring_buf(str1), kstring_buf(str2), min_size);

    return (res < 0 || (res == 0 && size1 < size2));
}

bool kstring_lep(TValue str1, TValue str2) { return !kstring_ltp(str2, str1); }
bool kstring_gtp(TValue str1, TValue str2) { return kstring_ltp(str2, str1); }
bool kstring_gep(TValue str1, TValue str2) { return !kstring_ltp(str1, str2); }

bool kstring_ci_ltp(TValue str1, TValue str2)
{
    int32_t size1 = kstring_size(str1);
    int32_t size2 = kstring_size(str2);
    int32_t min_size = size1 < size2? size1 : size2;
    char *buf1 = kstring_buf(str1);
    char *buf2 = kstring_buf(str2);

    while(min_size--) {
        int diff = (int) tolower(*buf1) - (int) tolower(*buf2);
        if (diff > 0)
            return false;
        else if (diff < 0)
            return true;
        buf1++, buf2++;
    }
    return size1 < size2;
}

bool kstring_ci_lep(TValue str1, TValue str2)
{
    return !kstring_ci_ltp(str2, str1);
}

bool kstring_ci_gtp(TValue str1, TValue str2)
{
    return kstring_ci_ltp(str2, str1);
}

bool kstring_ci_gep(TValue str1, TValue str2)
{
    return !kstring_ci_ltp(str1, str2);
}

/* 13.2.5? substring */
/* TEMP: at least for now this always returns mutable strings (like in Racket and
   following the Kernel Report where it says that object returned should be mutable 
   unless stated) */
void substring(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, ptree, "string", ttisstring, str,
             "exact integer", keintegerp, tv_start,
             "exact integer", keintegerp, tv_end);

    if (!ttisfixint(tv_start) || ivalue(tv_start) < 0 ||
        ivalue(tv_start) > kstring_size(str)) {
        /* TODO show index */
        klispE_throw_simple(K, "start index out of bounds");
        return;
    } 

    int32_t start = ivalue(tv_start);

    if (!ttisfixint(tv_end) || ivalue(tv_end) < 0 || 
        ivalue(tv_end) > kstring_size(str)) {
        klispE_throw_simple(K, "end index out of bounds");
        return;
    }

    int32_t end = ivalue(tv_end);

    if (start > end) {
        /* TODO show indexes */
        klispE_throw_simple(K, "end index is smaller than start index");
        return;
    }

    int32_t size = end - start;
    TValue new_str;
    /* the if isn't strictly necessary but it's clearer this way */
    if (size == 0) {
        new_str = G(K)->empty_string;
    } else {
        /* always returns mutable strings */
        new_str = kstring_new_bs(K, kstring_buf(str)+start, size);
    }
    kapply_cc(K, new_str);
}

/* 13.2.6? string-append */
/* TEMP: at least for now this always returns mutable strings */
/* TEMP: this does 3 passes over the list */
void string_append(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    /* don't allow cycles */
    int32_t pairs;
    check_typed_list(K, kstringp, false, ptree, &pairs, NULL);

    TValue new_str;
    int64_t total_size = 0; /* use int64 to check for overflow */
    /* the if isn't strictly necessary but it's clearer this way */
    int32_t saved_pairs = pairs; /* save pairs for next loop */
    TValue tail = ptree;
    while(pairs--) {
        total_size += kstring_size(kcar(tail));
        if (total_size > INT32_MAX) {
            klispE_throw_simple(K, "resulting string is too big");
            return;
        }
        tail = kcdr(tail);
    }
    /* this is safe */
    int32_t size = (int32_t) total_size;

    if (size == 0) {
        new_str = G(K)->empty_string; 
    } else {
        new_str = kstring_new_s(K, size);
        char *buf = kstring_buf(new_str);
        /* loop again to copy the chars of each string */
        tail = ptree;
        pairs = saved_pairs;

        while(pairs--) {
            TValue first = kcar(tail);
            int32_t first_size = kstring_size(first);
            memcpy(buf, kstring_buf(first), first_size);
            buf += first_size;
            tail = kcdr(tail);
        }
    }

    kapply_cc(K, new_str);
}


/* 13.2.7? string->list, list->string */
void string_to_list(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "string", ttisstring, str);
    TValue res = string_to_list_h(K, str, NULL);
    kapply_cc(K, res);
}

void list_to_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    /* check later */
    bind_1p(K, ptree, ls);
    /* don't allow cycles */
    int32_t pairs;
    check_typed_list(K, kcharp, false, ls, &pairs, NULL);
    TValue new_str = list_to_string_h(K, ls, pairs);
    kapply_cc(K, new_str);
}

/* 13.? string->vector, vector->string */
void string_to_vector(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "string", ttisstring, str);
    TValue res;

    if (kstring_emptyp(str)) {
        res = G(K)->empty_vector;
    } else {
        uint32_t size = kstring_size(str);

        /* MAYBE add vector constructor without fill */
        /* no need to root this */
        res = kvector_new_sf(K, size, KINERT);
        char *src = kstring_buf(str);
        TValue *dst = kvector_buf(res);
        while(size--) {
            char ch = *src++; /* not needed but just in case */
            *dst++ = ch2tv(ch); 
        }
    }
    kapply_cc(K, res);
}

/* TEMP Only ASCII for now */
void vector_to_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "vector", ttisvector, vec);
    TValue res;

    if (kvector_emptyp(vec)) {
        res = G(K)->empty_string;
    } else {
        uint32_t size = kvector_size(vec);

        res = kstring_new_s(K, size); /* no need to root this */
        TValue *src = kvector_buf(vec);
        char *dst = kstring_buf(res);
        while(size--) {
            TValue tv = *src++;
            if (!ttischar(tv)) {
                klispE_throw_simple_with_irritants(K, "Non char object found", 
                                                   1, tv);
                return;
            }
            *dst++ = chvalue(tv);
        }
    }
    kapply_cc(K, res);
}

/* 13.? string->bytevector, bytevector->string */
void string_to_bytevector(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "string", ttisstring, str);
    TValue res;

    if (kstring_emptyp(str)) {
        res = G(K)->empty_bytevector;
    } else {
        uint32_t size = kstring_size(str);

        /* MAYBE add bytevector constructor without fill */
        /* no need to root this */
        res = kbytevector_new_s(K, size);
        char *src = kstring_buf(str);
        uint8_t *dst = kbytevector_buf(res);
	
        while(size--) {
            *dst++ = (uint8_t)*src++; 
        }
    }
    kapply_cc(K, res);
}

/* TEMP Only ASCII for now */
void bytevector_to_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "bytevector", ttisbytevector, bb);
    TValue res;

    if (kbytevector_emptyp(bb)) {
        res = G(K)->empty_string;
    } else {
        uint32_t size = kbytevector_size(bb);
        res = kstring_new_s(K, size); /* no need to root this */
        uint8_t *src = kbytevector_buf(bb);
        char *dst = kstring_buf(res);
        while(size--) {
            uint8_t u8 = *src++;
            if (u8 >= 128) {
                klispE_throw_simple_with_irritants(K, "Char out of range", 
                                                   1, i2tv(u8));
                return;
            }
            *dst++ = (char) u8;
        }
    }
    kapply_cc(K, res);
}

/* 13.2.8? string-copy */
/* TEMP: at least for now this always returns mutable strings */
void string_copy(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);

    TValue new_str;
    /* the if isn't strictly necessary but it's clearer this way */
    if (tv_equal(str, G(K)->empty_string)) {
        new_str = str; 
    } else {
        new_str = kstring_new_bs(K, kstring_buf(str), kstring_size(str));
    }
    kapply_cc(K, new_str);
}

/* 13.2.9? string->immutable-string */
void string_to_immutable_string(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, str);

    TValue res_str;
    if (kstring_immutablep(str)) {/* this includes the empty list */
        res_str = str;
    } else {
        res_str = kstring_new_bs_imm(K, kstring_buf(str), kstring_size(str));
    }
    kapply_cc(K, res_str);
}

/* 13.2.10? string-fill! */
void string_fillB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "string", ttisstring, str,
             "char", ttischar, tv_ch);

    if (kstring_immutablep(str)) {
        klispE_throw_simple(K, "immutable string");
        return;
    } 

    memset(kstring_buf(str), chvalue(tv_ch), kstring_size(str));
    kapply_cc(K, KINERT);
}

/* init ground */
void kinit_strings_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from r5rs scheme and should not be considered standard. They are
    ** provided in the meantime to allow programs to use string features
    ** (ASCII only). 
    */

    /* 13.1.1? string? */
    add_applicative(K, ground_env, "string?", typep, 2, symbol, 
                    i2tv(K_TSTRING));
    /* 13.? immutable-string?, mutable-string? */
    add_applicative(K, ground_env, "immutable-string?", ftypep, 2, symbol, 
                    p2tv(kimmutable_stringp));
    add_applicative(K, ground_env, "mutable-string?", ftypep, 2, symbol, 
                    p2tv(kmutable_stringp));
    /* 13.1.2? make-string */
    add_applicative(K, ground_env, "make-string", make_string, 0);
    /* 13.1.3? string-length */
    add_applicative(K, ground_env, "string-length", string_length, 0);
    /* 13.1.4? string-ref */
    add_applicative(K, ground_env, "string-ref", string_ref, 0);
    /* 13.1.5? string-set! */
    add_applicative(K, ground_env, "string-set!", string_setB, 0);
    /* 13.2.1? string */
    add_applicative(K, ground_env, "string", string, 0);
    /* 13.?? string-upcase, string-downcase, string-titlecase, 
       string-foldcase */
    add_applicative(K, ground_env, "string-upcase", kstring_change_case, 1,
                    p2tv(toupper));
    add_applicative(K, ground_env, "string-downcase", kstring_change_case, 1,
                    p2tv(tolower));
    add_applicative(K, ground_env, "string-titlecase", kstring_title_case, 0);
    add_applicative(K, ground_env, "string-foldcase", kstring_change_case, 1,
                    p2tv(tolower));
    /* 13.2.2? string=?, string-ci=? */
    add_applicative(K, ground_env, "string=?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_eqp));
    add_applicative(K, ground_env, "string-ci=?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_ci_eqp));
    /* 13.2.3? string<?, string<=?, string>?, string>=? */
    add_applicative(K, ground_env, "string<?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_ltp));
    add_applicative(K, ground_env, "string<=?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_lep));
    add_applicative(K, ground_env, "string>?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_gtp));
    add_applicative(K, ground_env, "string>=?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_gep));
    /* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
    add_applicative(K, ground_env, "string-ci<?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_ci_ltp));
    add_applicative(K, ground_env, "string-ci<=?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_ci_lep));
    add_applicative(K, ground_env, "string-ci>?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_ci_gtp));
    add_applicative(K, ground_env, "string-ci>=?", ftyped_bpredp, 3,
                    symbol, p2tv(kstringp), p2tv(kstring_ci_gep));
    /* 13.2.5? substring */
    add_applicative(K, ground_env, "substring", substring, 0);
    /* 13.2.6? string-append */
    add_applicative(K, ground_env, "string-append", string_append, 0);
    /* 13.2.7? string->list, list->string */
    add_applicative(K, ground_env, "string->list", string_to_list, 0);
    add_applicative(K, ground_env, "list->string", list_to_string, 0);
    /* 13.?? string->vector, vector->string */
    add_applicative(K, ground_env, "string->vector", string_to_vector, 0);
    add_applicative(K, ground_env, "vector->string", vector_to_string, 0);
    /* 13.?? string->bytevector, bytevector->string */
    add_applicative(K, ground_env, "string->bytevector", 
                    string_to_bytevector, 0);
    add_applicative(K, ground_env, "bytevector->string", 
                    bytevector_to_string, 0);
    /* 13.2.8? string-copy */
    add_applicative(K, ground_env, "string-copy", string_copy, 0);
    /* 13.2.9? string->immutable-string */
    add_applicative(K, ground_env, "string->immutable-string", 
                    string_to_immutable_string, 0);

    /* 13.2.10? string-fill! */
    add_applicative(K, ground_env, "string-fill!", string_fillB, 0);
}
