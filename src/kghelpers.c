/*
** kghelpers.c
** Helper macros and functions for the ground environment
** See Copyright Notice in klisp.h
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "kghelpers.h"
#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kerror.h"
#include "ksymbol.h"
#include "kenvironment.h"
#include "kinteger.h"
#include "krational.h"
#include "kapplicative.h"
#include "kbytevector.h"
#include "kvector.h"
#include "kstring.h"
#include "kpair.h"
#include "kcontinuation.h"
#include "kencapsulation.h"
#include "kpromise.h"

/* Initialization of continuation names */
void kinit_kghelpers_cont_names(klisp_State *K)
{
    Table *t = tv2table(K->cont_name_table);
    add_cont_name(K, t, do_seq, "eval-sequence");
    add_cont_name(K, t, do_pass_value, "pass-value");
    add_cont_name(K, t, do_return_value, "return-value");
    add_cont_name(K, t, do_bind, "dynamic-bind");
    add_cont_name(K, t, do_bind, "dynamic-access");
    add_cont_name(K, t, do_bind, "dynamic-unbind");
    add_cont_name(K, t, do_bind, "dynamic-set!-pass");
}

/* Type predicates */
/* TODO these should be moved to either kobject.h or the corresponding
   files (e.g. kbooleanp to kboolean.h */
bool kbooleanp(TValue obj) { return ttisboolean(obj); }
bool kcombinerp(TValue obj) { return ttiscombiner(obj); }
bool knumberp(TValue obj) { return ttisnumber(obj); }
/* TEMP used (as a type predicate) in all predicates that need a primary value
   (XXX it's not actually a type error, but it's close enough and otherwise 
   should define new predp & bpredp for numeric predicates...) */
bool knumber_wpvp(TValue obj) 
{ 
    return ttisnumber(obj) && !ttisrwnpv(obj) && !ttisundef(obj); 
}
/* This is used in gcd & lcm */
bool kimp_intp(TValue obj) { return ttisinteger(obj) || ttisinf(obj); }
/* obj is known to be a number */
bool kfinitep(TValue obj) { return !ttisinf(obj); }
/* fixint, bigints & inexact integers */
bool kintegerp(TValue obj) { return ttisinteger(obj); }
/* only exact integers (like for indices), bigints & fixints */
bool keintegerp(TValue obj) { return ttiseinteger(obj); }
/* exact integers between 0 and 255 inclusive */
bool ku8p(TValue obj) { return ttisu8(obj); }
bool krationalp(TValue obj) { return ttisrational(obj); }
bool krealp(TValue obj) { return ttisreal(obj); }
/* TEMP used (as a type predicate) in all predicates that need a real with 
   primary value (XXX it's not actually a type error, but it's close enough 
   and otherwise should define new predp & bpredp for numeric predicates...) */
bool kreal_wpvp(TValue obj) { return ttisreal(obj) && !ttisrwnpv(obj); }

bool kexactp(TValue obj) { return ttisexact(obj); }
bool kinexactp(TValue obj) { return ttisinexact(obj); }
bool kundefinedp(TValue obj) { return ttisundef(obj); }
bool krobustp(TValue obj) { return ttisrobust(obj); }

void enc_typep(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    /*
    ** xparams[0]: encapsulation key
    */
    TValue key = xparams[0];

    /* check the ptree is a list while checking the predicate.
       Keep going even if the result is false to catch errors in 
       ptree structure */
    bool res = true;

    TValue tail = ptree;
    while(ttispair(tail) && kis_unmarked(tail)) {
        kmark(tail);
        res &= kis_encapsulation_type(kcar(tail), key);
        tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttispair(tail) || ttisnil(tail)) {
        kapply_cc(K, b2tv(res));
    } else {
        /* try to get name from encapsulation */
        klispE_throw_simple(K, "expected list");
        return;
    }
}
/* /Type predicates */

/* some number functions */
bool kpositivep(TValue n)
{ 
    switch (ttype(n)) {
    case K_TFIXINT:
    case K_TEINF:
    case K_TIINF:
        return ivalue(n) > 0;
    case K_TBIGINT:
        return kbigint_positivep(n);
    case K_TBIGRAT:
        return kbigrat_positivep(n);
    case K_TDOUBLE:
        return dvalue(n) > 0.0;
        /* real with no prim value, complex and undefined should be captured by 
           type predicate */
    default:
        klisp_assert(0);
        return false;
    }
}

bool knegativep(TValue n) 
{ 
    switch (ttype(n)) {
    case K_TFIXINT:
    case K_TEINF:
    case K_TIINF:
        return ivalue(n) < 0;
    case K_TBIGINT:
        return kbigint_negativep(n);
    case K_TBIGRAT:
        return kbigrat_negativep(n);
    case K_TDOUBLE:
        return dvalue(n) < 0.0;
        /* real with no prim value, complex and undefined should be captured by 
           type predicate */
    default:
        klisp_assert(0);
        return false;
    }
}
/* /some number functions */

void typep(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type tag (as by i2tv)
    */
    UNUSED(denv);
    int32_t tag = ivalue(xparams[1]);

    /* check the ptree is a list while checking the predicate.
       Keep going even if the result is false to catch errors in 
       ptree structure */
    bool res = true;

    TValue tail = ptree;
    while(ttispair(tail) && kis_unmarked(tail)) {
        kmark(tail);
        res &= ttype(kcar(tail)) == tag;
        tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttispair(tail) || ttisnil(tail)) {
        kapply_cc(K, b2tv(res));
    } else {
        klispE_throw_simple(K, "expected list");
        return;
    }
}

void ftypep(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: fn pointer (as a void * in a user TValue)
    */
    bool (*fn)(TValue obj) = pvalue(xparams[1]);

    /* check the ptree is a list while checking the predicate.
       Keep going even if the result is false to catch errors in 
       ptree structure */
    bool res = true;

    TValue tail = ptree;
    while(ttispair(tail) && kis_unmarked(tail)) {
        kmark(tail);
        res &= (*fn)(kcar(tail));
        tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttispair(tail) || ttisnil(tail)) {
        kapply_cc(K, b2tv(res));
    } else {
        klispE_throw_simple(K, "expected list");
        return;
    }
}

/*
** REFACTOR: Change this to make it a single pass
*/
void ftyped_predp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type fn pointer (as a void * in a user TValue)
    ** xparams[2]: fn pointer (as a void * in a user TValue)
    */
    bool (*typep)(TValue obj) = pvalue(xparams[1]);
    bool (*predp)(TValue obj) = pvalue(xparams[2]);

    /* check the ptree is a list first to allow the structure
       errors to take precedence over the type errors. */
    int32_t pairs, cpairs;
    check_list(K, true, ptree, &pairs, &cpairs);

    TValue tail = ptree;
    bool res = true;

    /* check the type while checking the predicate.
       Keep going even if the result is false to catch errors in 
       type */
    while(pairs--) {
        TValue first = kcar(tail);

        if (!(*typep)(first)) {
            /* TODO show expected type */
            klispE_throw_simple(K, "bad argument type");
            return;
        }
        res &= (*predp)(first);
        tail = kcdr(tail);
    }
    kapply_cc(K, b2tv(res));
}

/*
** REFACTOR: Change this to make it a single pass
*/
void ftyped_bpredp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type fn pointer (as a void * in a user TValue)
    ** xparams[2]: fn pointer (as a void * in a user TValue)
    */
    bool (*typep)(TValue obj) = pvalue(xparams[1]);
    bool (*predp)(TValue obj1, TValue obj2) = pvalue(xparams[2]);

    /* check the ptree is a list first to allow the structure
       errors to take precedence over the type errors. */
    int32_t pairs, cpairs;
    check_list(K, true, ptree, &pairs, &cpairs);

    /* cyclical list require an extra comparison of the last
       & first element of the cycle */
    int32_t comps = cpairs? pairs : pairs - 1;

    TValue tail = ptree;
    bool res = true;

    /* check the type while checking the predicate.
       Keep going even if the result is false to catch errors in 
       type */

    if (comps == 0) {
        /* this case has to be here because otherwise there is no check
           for the type of the lone operand */
        TValue first = kcar(tail);
        if (!(*typep)(first)) {
            /* TODO show expected type */
            klispE_throw_simple(K, "bad argument type");
            return;
        }
    }

    while(comps-- > 0) { /* comps could be -1 if ptree is () */
        TValue first = kcar(tail);
        tail = kcdr(tail); /* tail only advances one place per iteration */
        TValue second = kcar(tail);

        if (!(*typep)(first) || !(*typep)(second)) {
            /* TODO show expected type */
            klispE_throw_simple(K, "bad argument type");
            return;
        }
        res &= (*predp)(first, second);
    }
    kapply_cc(K, b2tv(res));
}

/* This is the same, but the comparison predicate takes a klisp_State */
/* TODO unify them */
void ftyped_kbpredp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type fn pointer (as a void * in a user TValue)
    ** xparams[2]: fn pointer (as a void * in a user TValue)
    */
    bool (*typep)(TValue obj) = pvalue(xparams[1]);
    bool (*predp)(klisp_State *K, TValue obj1, TValue obj2) = 
        pvalue(xparams[2]);

    /* check the ptree is a list first to allow the structure
       errors to take precedence over the type errors. */
    int32_t pairs, cpairs;
    check_list(K, true, ptree, &pairs, &cpairs);

    /* cyclical list require an extra comparison of the last
       & first element of the cycle */
    int32_t comps = cpairs? pairs : pairs - 1;

    TValue tail = ptree;
    bool res = true;

    /* check the type while checking the predicate.
       Keep going even if the result is false to catch errors in 
       type */

    if (comps == 0) {
        /* this case has to be here because otherwise there is no check
           for the type of the lone operand */
        TValue first = kcar(tail);
        if (!(*typep)(first)) {
            /* TODO show expected type */
            klispE_throw_simple(K, "bad argument type");
            return;
        }
    }

    while(comps-- > 0) { /* comps could be -1 if ptree is () */
        TValue first = kcar(tail);
        tail = kcdr(tail); /* tail only advances one place per iteration */
        TValue second = kcar(tail);

        if (!(*typep)(first) || !(*typep)(second)) {
            /* TODO show expected type */
            klispE_throw_simple(K, "bad argument type");
            return;
        }
        res &= (*predp)(K, first, second);
    }
    kapply_cc(K, b2tv(res));
}

/* typed finite list. Structure error should be throw before type errors */
void check_typed_list(klisp_State *K, bool (*typep)(TValue), bool allow_infp, 
                      TValue obj, int32_t *pairs, int32_t *cpairs)
{
    TValue tail = obj;
    int32_t p = 0;
    bool type_errorp = false;

    while(ttispair(tail) && !kis_marked(tail)) {
        /* even if there is a type error continue checking the structure */
        type_errorp |= !(*typep)(kcar(tail));
        kset_mark(tail, i2tv(p));
        tail = kcdr(tail);
        ++p;
    }

    if (pairs != NULL) *pairs = p;
    if (cpairs != NULL)
        *cpairs = ttispair(tail)? (p - ivalue(kget_mark(tail))) : 0;

    unmark_list(K, obj);

    if (!ttispair(tail) && !ttisnil(tail)) {
        klispE_throw_simple(K, allow_infp? "expected list" :
                            "expected finite list"); 
        return;
    } else if(ttispair(tail) && !allow_infp) {
        klispE_throw_simple(K, "expected finite list"); 
        return;
    } else if (type_errorp) {
        /* TODO put type name too, should be extracted from a
           table of type names */
        klispE_throw_simple(K, "bad operand type"); 
        return;
    }
}

void check_list(klisp_State *K, bool allow_infp, TValue obj, 
                int32_t *pairs, int32_t *cpairs)
{
    TValue tail = obj;
    int32_t p = 0;

    while(ttispair(tail) && !kis_marked(tail)) {
        kset_mark(tail, i2tv(p));
        tail = kcdr(tail);
        ++p;
    }

    if (pairs != NULL) *pairs = p;
    if (cpairs != NULL)
        *cpairs = ttispair(tail)? (p - ivalue(kget_mark(tail))) : 0;

    unmark_list(K, obj);

    if (!ttispair(tail) && !ttisnil(tail)) {
        klispE_throw_simple(K, allow_infp? "expected list" : 
                            "expected finite list"); 
        return;
    } else if(ttispair(tail) && !allow_infp) {
        klispE_throw_simple(K, "expected finite list"); 
        return;
    }
}


TValue check_copy_list(klisp_State *K, TValue obj, bool force_copy, 
                       int32_t *pairs, int32_t *cpairs)
{
    int32_t p = 0;
    if (ttisnil(obj)) {
        if (pairs != NULL) *pairs = 0;
        if (cpairs != NULL) *cpairs = 0;
        return obj;
    }

    if (ttispair(obj) && kis_immutable(obj) && !force_copy) {
        /* this will properly set pairs and cpairs */
        check_list(K, true, obj, pairs, cpairs);
        return obj;
    } else {
        TValue copy = kcons(K, KNIL, KNIL);
        krooted_vars_push(K, &copy);
        TValue last_pair = copy;
        TValue tail = obj;
    
        while(ttispair(tail) && !kis_marked(tail)) {
            TValue new_pair = kcons(K, kcar(tail), KNIL);
            /* record the corresponding pair to simplify cycle handling */
            kset_mark(tail, new_pair);
            /* record the pair number in the new pair, to set cpairs */
            kset_mark(new_pair, i2tv(p));
            /* copy the source code info */
            TValue si = ktry_get_si(K, tail);
            if (!ttisnil(si))
                kset_source_info(K, new_pair, si);
            kset_cdr(last_pair, new_pair);
            last_pair = new_pair;
            tail = kcdr(tail);
            ++p;
        }

        if (pairs != NULL) *pairs = p;
        if (cpairs != NULL)
            *cpairs = ttispair(tail)? 
                (p - ivalue(kget_mark(kget_mark(tail)))) : 
                0;

        if (ttispair(tail)) {
            /* complete the cycle */
            kset_cdr(last_pair, kget_mark(tail));
        }

        unmark_list(K, obj);
        unmark_list(K, kcdr(copy));

        if (!ttispair(tail) && !ttisnil(tail)) {
            klispE_throw_simple(K, "expected list"); 
            return KINERT;
        } 
        krooted_vars_pop(K);
        return kcdr(copy);
    }
}

/* GC: assumes ls is rooted */
TValue reverse_copy_and_encycle(klisp_State *K, TValue ls, int32_t pairs, 
				int32_t cpairs)
{
    if (pairs == 0)
        return KNIL;
    
    int32_t apairs = pairs - cpairs;
    TValue last = kcons(K, kcar(ls), KNIL);
    ls = kcdr(ls);
    krooted_vars_push(K, &last);

    if (cpairs > 0) {
        --cpairs;
	TValue last_cycle = last;
	while (cpairs > 0) {
	    last = kcons(K, kcar(ls), last);
	    ls = kcdr(ls);
	    --cpairs;
	}
	kset_cdr(last_cycle, last);
    } else {
        --apairs;
    }
    
    while (apairs > 0) {
	last = kcons(K, kcar(ls), last);
	ls = kcdr(ls);
	--apairs;
    }

    krooted_vars_pop(K);
    return last;
}

TValue check_copy_env_list(klisp_State *K, TValue obj)
{
    TValue copy = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &copy);
    TValue last_pair = copy;
    TValue tail = obj;
    
    while(ttispair(tail) && !kis_marked(tail)) {
        TValue first = kcar(tail);
        if (!ttisenvironment(first)) {
            klispE_throw_simple(K, "not an environment in parent list");
            return KINERT;
        }
        TValue new_pair = kcons(K, first, KNIL);
        kmark(tail);
        kset_cdr(last_pair, new_pair);
        last_pair = new_pair;
        tail = kcdr(tail);
    }

    /* even if there was a cycle, the copy ends with nil */
    unmark_list(K, obj);

    if (!ttispair(tail) && !ttisnil(tail)) {
        klispE_throw_simple(K, "expected list"); 
        return KINERT;
    } 
    krooted_vars_pop(K);
    return kcdr(copy);
}

/* Helpers for string, list->string, and string-map,
   bytevector, list->bytevector, bytevector-map, 
   vector, list->vector, and vector-map */
/* GC: Assume ls is rooted */
/* ls should a list of length 'length' of the correct type 
   (chars for string, u8 for bytevector, any for vector) */
/* these type checks each element */

TValue list_to_string_h(klisp_State *K, TValue ls, int32_t length)
{
    TValue new_str;
    /* the if isn't strictly necessary but it's clearer this way */
    if (length == 0) {
        return K->empty_string; 
    } else {
        new_str = kstring_new_s(K, length);
        char *buf = kstring_buf(new_str);
        while(length-- > 0) {
            TValue head = kcar(ls);
            if (!ttischar(head)) {
                klispE_throw_simple_with_irritants(K, "Bad type (expected "
                                                   "char)", 1, head);
                return KINERT;
            }
            *buf++ = chvalue(head);
            ls = kcdr(ls);
        }
        return new_str;
    }
}

TValue list_to_vector_h(klisp_State *K, TValue ls, int32_t length)
{

    if (length == 0) {
        return K->empty_vector;
    } else {
        TValue new_vec = kvector_new_sf(K, length, KINERT);
        TValue *buf = kvector_buf(new_vec);
        while(length-- > 0) {
            *buf++ = kcar(ls);
            ls = kcdr(ls);
        }
        return new_vec;
    }
}

TValue list_to_bytevector_h(klisp_State *K, TValue ls, int32_t length)
{
    TValue new_bb;
    /* the if isn't strictly necessary but it's clearer this way */
    if (length == 0) {
        return K->empty_bytevector; 
    } else {
        new_bb = kbytevector_new_s(K, length);
        uint8_t *buf = kbytevector_buf(new_bb);
        while(length-- > 0) {
            TValue head = kcar(ls);
            if (!ttisu8(head)) {
                klispE_throw_simple_with_irritants(K, "Bad type (expected "
                                                   "u8)", 1, head);
                return KINERT;
            }
            *buf++ = ivalue(head);
            ls = kcdr(ls);
        }
        return new_bb;
    }
}

/* Helpers for string->list, string-map, string-foreach,
   bytevector->list, bytevector-map, bytevector-foreach,
   vector->list, vector-map, and vector-foreach */
/* GC: Assume array is rooted */
TValue string_to_list_h(klisp_State *K, TValue obj, int32_t *length)
{
    if (!ttisstring(obj)) {
        klispE_throw_simple_with_irritants(K, "Bad type (expected string)",
                                           1, obj);
        return KINERT;
    }

    int32_t pairs = kstring_size(obj);
    if (length != NULL)	*length = pairs;

    char *buf = kstring_buf(obj) + pairs - 1;
    TValue tail = KNIL;
    krooted_vars_push(K, &tail);
    while(pairs-- > 0) {
        tail = kcons(K, ch2tv(*buf), tail);
        --buf;
    }
    krooted_vars_pop(K);
    return tail;
}

TValue vector_to_list_h(klisp_State *K, TValue obj, int32_t *length)
{
    if (!ttisvector(obj)) {
        klispE_throw_simple_with_irritants(K, "Bad type (expected vector)",
                                           1, obj);
        return KINERT;
    }

    int32_t pairs = kvector_size(obj);
    if (length != NULL)	*length = pairs;

    TValue *buf = kvector_buf(obj) + pairs - 1;
    TValue tail = KNIL;
    krooted_vars_push(K, &tail);
    while(pairs-- > 0) {
        tail = kcons(K, *buf, tail);
        --buf;
    }
    krooted_vars_pop(K);
    return tail;
}

TValue bytevector_to_list_h(klisp_State *K, TValue obj, int32_t *length)
{
    if (!ttisbytevector(obj)) {
        klispE_throw_simple_with_irritants(K, "Bad type (expected bytevector)",
                                           1, obj);
        return KINERT;
    }

    int32_t pairs = kbytevector_size(obj);
    if (length != NULL)	*length = pairs;

    uint8_t *buf = kbytevector_buf(obj) + pairs - 1;
    TValue tail = KNIL;
    krooted_vars_push(K, &tail);
    while(pairs-- > 0) {
        tail = kcons(K, i2tv(*buf), tail);
        --buf;
    }
    krooted_vars_pop(K);
    return tail;
}

/* Some helpers for working with fixints (signed 32 bits) */
int64_t kgcd32_64(int32_t a_, int32_t b_)
{
    /* this is a vanilla binary gcd algorithm */ 

    /* work with positive numbers, use unsigned numbers to 
       allow INT32_MIN to have an absolute value */
    uint32_t a = (uint32_t) kabs64(a_);
    uint32_t b = (uint32_t) kabs64(b_);

    int powerof2;

    /* the easy cases first, unlike the general kernel gcd the
       gcd2 of a number and zero is zero */
    if (a == 0)
        return (int64_t) b;
    else if (b == 0)
        return (int64_t) a;
 
    for (powerof2 = 0; ((a & 1) == 0) && 
             ((b & 1) == 0); ++powerof2, a >>= 1, b >>= 1)
        ;
 
    while(a != 0 && b!= 0) {
        /* either a or b are odd, make them both odd */
        for (; (a & 1) == 0; a >>= 1)
            ;
        for (; (b & 1) == 0; b >>= 1)
            ;

        /* now the difference is sure to be even */
        if (a < b) {
            b = (b - a) >> 1;
        } else {
            a = (a - b) >> 1;
        }
    }
 
    return ((int64_t) (a == 0? b : a)) << powerof2;
}

int64_t klcm32_64(int32_t a_, int32_t b_)
{
    int64_t gcd = kgcd32_64(a_, b_);
    int64_t a = kabs64(a_);
    int64_t b = kabs64(b_);
    /* divide first to avoid possible overflow */
    return (a / gcd) * b;
}

/* This is needed in kstate & promises */
void memoize(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1p(K, ptree, exp);
    TValue new_prom = kmake_promise(K, exp, KNIL);
    kapply_cc(K, new_prom);
}

/* list applicative (used in kstate and kgpairs_lists) */
void list(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
/* the underlying combiner of list return the complete ptree, the only list
   checking is implicit in the applicative evaluation */
    UNUSED(xparams);
    UNUSED(denv);
    kapply_cc(K, ptree);
}

/* Helper for get-list-metrics, and list-tail, list-ref and list-set! 
   when receiving bigint indexes */
void get_list_metrics_aux(klisp_State *K, TValue obj, int32_t *p, int32_t *n, 
                          int32_t *a, int32_t *c)
{
    TValue tail = obj;
    int32_t pairs = 0;

    while(ttispair(tail) && !kis_marked(tail)) {
        /* record the pair number to simplify cycle pair counting */
        kset_mark(tail, i2tv(pairs));
        ++pairs;
        tail = kcdr(tail);
    }
    int32_t apairs, cpairs, nils;
    if (ttisnil(tail)) {
        /* simple (possibly empty) list */
        apairs = pairs;
        nils = 1;
        cpairs = 0;
    } else if (ttispair(tail)) {
        /* cyclic (maybe circular) list */
        apairs = ivalue(kget_mark(tail));
        cpairs = pairs - apairs;
        nils = 0;
    } else {
        apairs = pairs;
        cpairs = 0;
        nils = 0;
    }

    unmark_list(K, obj);

    if (p != NULL) *p = pairs;
    if (n != NULL) *n = nils;
    if (a != NULL) *a = apairs;
    if (c != NULL) *c = cpairs;
}

/* Helper for list-tail, list-ref and list-set! */
/* Calculate the smallest i such that 
   (eq? (list-tail obj i) (list-tail obj tk))
   tk is a bigint and all lists have fixint range number of pairs,
   so the list should cyclic and we should calculate an index that
   doesn't go through the complete cycle not even once */
int32_t ksmallest_index(klisp_State *K, TValue obj, TValue tk)
{
    int32_t apairs, cpairs;
    get_list_metrics_aux(K, obj, NULL, NULL, &apairs, &cpairs);
    if (cpairs == 0) {
        klispE_throw_simple(K, "non pair found while traversing "
                            "object");
        return 0;
    }
    TValue tv_apairs = i2tv(apairs);
    TValue tv_cpairs = i2tv(cpairs);
	
    /* all calculations will be done with bigints */
    kensure_bigint(tv_apairs);
    kensure_bigint(tv_cpairs);
	
    TValue idx = kbigint_minus(K, tk, tv_apairs);
    krooted_tvs_push(K, idx); /* root idx if it is a bigint */
    /* idx may have become a fixint */
    kensure_bigint(idx);
    UNUSED(kbigint_div_mod(K, idx, tv_cpairs, &idx));
    krooted_tvs_pop(K);
    /* now idx is less than cpairs so it fits in a fixint */
    assert(ttisfixint(idx));
    return ivalue(idx) + apairs; 
}

/* Helper for eq? and equal? */
bool eq2p(klisp_State *K, TValue obj1, TValue obj2)
{
    bool res = (tv_equal(obj1, obj2));
    if (!res && (ttype(obj1) == ttype(obj2))) {
        switch (ttype(obj1)) {
        case K_TSYMBOL:
            /* symbols can't be compared with tv_equal! */
            res = tv_sym_equal(obj1, obj2);
            break;
        case K_TAPPLICATIVE:
            while(ttisapplicative(obj1) && ttisapplicative(obj2)) {
                obj1 = kunwrap(obj1);
                obj2 = kunwrap(obj2);
            }
            res = (tv_equal(obj1, obj2));
            break;
        case K_TBIGINT:
            /* it's important to know that it can't be the case
               that obj1 is bigint and obj is some other type and
               (eq? obj1 obj2) */
            res = kbigint_eqp(obj1, obj2);
            break;
        case K_TBIGRAT:
            /* it's important to know that it can't be the case
               that obj1 is bigrat and obj is some other type and
               (eq? obj1 obj2) */
            res = kbigrat_eqp(K, obj1, obj2);
            break;
        } /* immutable strings & bytevectors are interned so they are 
             covered already by tv_equalp */

    }
    return res;
}

/*
** Helpers for equal? algorithm
**
** See [2] for details of the list merging algorithm. 
** Here are the implementation details:
** The marks of the pairs are used to store the nodes of the trees 
** that represent the set of previous comparations of each pair. 
** They serve the function of the array in [2].
** If a pair is unmarked, it was never compared (empty comparison set). 
** If a pair is marked, the mark object is either (#f . parent-node) 
** if the node is not the root, and (#t . n) where n is the number 
** of elements in the set, if the node is the root. 
** This pair also doubles as the "name" of the set in [2].
**
** GC: all of these assume that arguments are rooted.
*/

/* find "name" of the set of this obj, if there isn't one create it,
   if there is one, flatten its branch */
static inline TValue equal_find(klisp_State *K, TValue obj)
{
    /* GC: should root obj */
    if (kis_unmarked(obj)) {
        /* object wasn't compared before, create new set */
        TValue new_node = kcons(K, KTRUE, i2tv(1));
        kset_mark(obj, new_node);
        return new_node;
    } else {		
        TValue node = kget_mark(obj);

        /* First obtain the root and a list of all the other objects in this 
           branch, as said above the root is the one with #t in its car */
        /* NOTE: the stack is being used, so we must remember how many pairs we 
           push, we can't just pop 'till is empty */
        int np = 0;
        while(kis_false(kcar(node))) {
            ks_spush(K, node);
            node = kcdr(node);
            ++np;
        }
        TValue root = node;

        /* set all parents to root, to flatten the branch */
        while(np--) {
            node = ks_spop(K);
            kset_cdr(node, root);
        }
        return root;
    }
}

/* merge the smaller set into the big one, if both are equal just pick one */
static inline void equal_merge(klisp_State *K, TValue root1, TValue root2)
{
    /* K isn't needed but added for consistency */
    UNUSED(K);
    int32_t size1 = ivalue(kcdr(root1));
    int32_t size2 = ivalue(kcdr(root2));
    TValue new_size = i2tv(size1 + size2);
    
    if (size1 < size2) {
        /* add root1 set (the smaller one) to root2 */
        kset_cdr(root2, new_size);
        kset_car(root1, KFALSE);
        kset_cdr(root1, root2);
    } else {
        /* add root2 set (the smaller one) to root1 */
        kset_cdr(root1, new_size);
        kset_car(root2, KFALSE);
        kset_cdr(root2, root1);
    }
}

/* check to see if two objects were already compared, and return that. If they
   weren't compared yet, merge their sets (and flatten their branches) */
static inline bool equal_find2_mergep(klisp_State *K, TValue obj1, TValue obj2)
{
    /* GC: should root root1 and root2 */
    TValue root1 = equal_find(K, obj1);
    TValue root2 = equal_find(K, obj2);
    if (tv_equal(root1, root2)) {
        /* they are in the same set => they were already compared */
        return true;
    } else {
        equal_merge(K, root1, root2);
        return false;
    }
}

/*
** See [1] for details, in this case the pairs form a possibly infinite "tree" 
** structure, and that can be seen as a finite automata, where each node is a 
** state, the car and the cdr are the transitions from that state to others, 
** and the leaves (the non-pair objects) are the final states.
** Other way to see it is that, the key for determining equalness of two pairs 
** is: Check to see if they were already compared to each other.
** If so, return #t, otherwise, mark them as compared to each other and 
** recurse on both cars and both cdrs.
** The idea is that if assuming obj1 and obj2 are equal their components are 
** equal then they are effectively equal to each other.
*/
bool equal2p(klisp_State *K, TValue obj1, TValue obj2)
{
    assert(ks_sisempty(K));

    /* the stack has the elements to be compaired, always in pairs.
       So the top should be compared with the one below, the third with
       the fourth and so on */
    ks_spush(K, obj1);
    ks_spush(K, obj2);

    /* if the stacks becomes empty, all pairs of elements were equal */
    bool result = true;
    TValue saved_obj1 = obj1;
    TValue saved_obj2 = obj2;

    while(!ks_sisempty(K)) {
        obj2 = ks_spop(K);
        obj1 = ks_spop(K);

        if (!eq2p(K, obj1, obj2)) {
            /* This type comparison works because we just care about
               pairs, vectors, strings & bytevectors */
            if (ttype(obj1) == ttype(obj2)) {
                switch(ttype(obj1)) {
                case K_TPAIR:
                    /* if they were already compaired, consider equal for 
                       now otherwise they are equal if both their cars 
                       and cdrs are */
                    if (!equal_find2_mergep(K, obj1, obj2)) {
                        ks_spush(K, kcdr(obj1));
                        ks_spush(K, kcdr(obj2));
                        ks_spush(K, kcar(obj1));
                        ks_spush(K, kcar(obj2));
                    }
                    break;
                case K_TVECTOR:
                    if (kvector_size(obj1) == kvector_size(obj2)) {
                        /* if they were already compaired, consider equal for 
                           now otherwise they are equal if all their elements
                           are equal pairwise */
                        if (!equal_find2_mergep(K, obj1, obj2)) {
                            uint32_t i = kvector_size(obj1);
                            TValue *array1 = kvector_buf(obj1);
                            TValue *array2 = kvector_buf(obj2);
                            while(i-- > 0) {
                                ks_spush(K, array1[i]);
                                ks_spush(K, array2[i]);
                            }
                        }
                    } else {
                        result = false;
                        goto end;
                    }
                    break;
                case K_TSTRING:
                    if (!kstring_equalp(obj1, obj2)) {
                        result = false;
                        goto end;
                    }
                    break;
                case K_TBYTEVECTOR:
                    if (!kbytevector_equalp(obj1, obj2)) {
                        result = false;
                        goto end;
                    }
                    break;
                default:
                    result = false;
                    goto end;
                }
            } else {
                result = false;
                goto end;
            }
        }
    }
end:
    /* if result is false, the stack may not be empty */
    ks_sclear(K);

    unmark_tree(K, saved_obj1);
    unmark_tree(K, saved_obj2);
    
    return result;
}

/*
** This is in a helper method to use it from $lambda, $vau, etc
**
** We mark each seen mutable pair with the corresponding copied 
** immutable pair to construct a structure that is isomorphic to 
** the original.
** All objects that aren't mutable pairs are retained without 
** copying
** sstack is used to keep track of pairs and tbstack is used
** to keep track of which of car or cdr we were copying,
** 0 means just pushed, 1 means return from car, 2 means return from cdr
**
** This also copies source code info
**
*/

/* GC: assumes obj is rooted */
TValue copy_es_immutable_h(klisp_State *K, TValue obj, bool mut_flag)
{
    TValue copy = obj;
    krooted_vars_push(K, &copy);

    assert(ks_sisempty(K));
    assert(ks_tbisempty(K));

    ks_spush(K, obj);
    ks_tbpush(K, ST_PUSH);

    while(!ks_sisempty(K)) {
        char state = ks_tbpop(K);
        TValue top = ks_spop(K);

        if (state == ST_PUSH) {
            /* if the pair is immutable & we are constructing immutable
               pairs there is no need to copy */
            if (ttispair(top) && (mut_flag || kis_mutable(top))) {
                if (kis_marked(top)) {
                    /* this pair was already seen, use the same */
                    copy = kget_mark(top);
                } else {
                    TValue new_pair = kcons_g(K, mut_flag, KINERT, KINERT);
                    kset_mark(top, new_pair);
                    /* save the source code info on the new pair */
                    /* MAYBE: only do it if mutable */
                    TValue si = ktry_get_si(K, top);
                    if (!ttisnil(si))
                        kset_source_info(K, new_pair, si);
                    /* leave the pair in the stack, continue with the car */
                    ks_spush(K, top);
                    ks_tbpush(K, ST_CAR);
		    
                    ks_spush(K, kcar(top));
                    ks_tbpush(K, ST_PUSH);
                }
            } else {
                copy = top;
            }
        } else { /* last action was a pop */
            TValue new_pair = kget_mark(top);
            if (state == ST_CAR) {
                /* new_pair may be immutable */
                kset_car_unsafe(K, new_pair, copy);
                /* leave the pair on the stack, continue with the cdr */
                ks_spush(K, top);
                ks_tbpush(K, ST_CDR);

                ks_spush(K, kcdr(top));
                ks_tbpush(K, ST_PUSH);
            } else {
                /* new_pair may be immutable */
                kset_cdr_unsafe(K, new_pair, copy);
                copy = new_pair;
            }
        }
    }
    unmark_tree(K, obj);
    krooted_vars_pop(K);
    return copy;
}

/* ptree handling */

/*
** Clear all the marks (symbols + pairs) & stacks.
** The stack should contain only pairs, sym_ls should be
** as above 
*/    
static inline void ptree_clear_all(klisp_State *K, TValue sym_ls)
{
    while(!ttisnil(sym_ls)) {
        TValue first = sym_ls;
        sym_ls = kget_symbol_mark(first);
        kunmark_symbol(first);
    }

    while(!ks_sisempty(K)) {
        kunmark(ks_sget(K));
        ks_sdpop(K);
    }

    ks_tbclear(K);
}

/* GC: assumes env, ptree & obj are rooted */
void match(klisp_State *K, TValue env, TValue ptree, TValue obj)
{
    assert(ks_sisempty(K));
    ks_spush(K, obj);
    ks_spush(K, ptree);

    while(!ks_sisempty(K)) {
        ptree = ks_spop(K);
        obj = ks_spop(K);

        switch(ttype(ptree)) {
        case K_TNIL:
            if (!ttisnil(obj)) {
                /* TODO show ptree and arguments */
                ks_sclear(K);
                klispE_throw_simple(K, "ptree doesn't match arguments");
                return;
            }
            break;
        case K_TIGNORE:
            /* do nothing */
            break;
        case K_TSYMBOL:
            kadd_binding(K, env, ptree, obj);
            break;
        case K_TPAIR:
            if (ttispair(obj)) {
                ks_spush(K, kcdr(obj));
                ks_spush(K, kcdr(ptree));
                ks_spush(K, kcar(obj));
                ks_spush(K, kcar(ptree));
            } else {
                /* TODO show ptree and arguments */
                ks_sclear(K);
                klispE_throw_simple(K, "ptree doesn't match arguments");
                return;
            }
            break;
        default:
            /* can't really happen */
            break;
        }
    }
}

/* GC: assumes ptree & penv are rooted */
TValue check_copy_ptree(klisp_State *K, TValue ptree, TValue penv)
{
    /* copy is only valid if the state isn't ST_PUSH */
    /* but init anyways for gc (and avoiding warnings) */
    TValue copy = ptree;
    krooted_vars_push(K, &copy);

    /* 
    ** NIL terminated singly linked list of symbols 
    ** (using the mark as next pointer) 
    */
    TValue sym_ls = KNIL;

    assert(ks_sisempty(K));
    assert(ks_tbisempty(K));

    ks_tbpush(K, ST_PUSH);
    ks_spush(K, ptree);

    while(!ks_sisempty(K)) {
        char state = ks_tbpop(K);
        TValue top = ks_spop(K);

        if (state == ST_PUSH) {
            switch(ttype(top)) {
            case K_TIGNORE:
            case K_TNIL:
                copy = top;
                break;
            case K_TSYMBOL: {
                if (kis_symbol_marked(top)) {
                    ptree_clear_all(K, sym_ls);
                    klispE_throw_simple_with_irritants(K, "repeated symbol "
                                                       "in ptree", 1, top);
                    return KNIL;
                } else {
                    copy = top;
                    /* add it to the symbol list */
                    kset_symbol_mark(top, sym_ls);
                    sym_ls = top;
                }
                break;
            }
            case K_TPAIR: {
                if (kis_unmarked(top)) {
                    if (kis_immutable(top)) {
                        /* don't copy mutable pairs, just use them */
                        /* NOTE: immutable pairs can't have mutable
                           car or cdr */
                        /* we have to continue thou, because there could be a 
                           cycle */
                        kset_mark(top, top);
                    } else {
                        /* create a new pair as copy, save it in the mark */
                        TValue new_pair = kimm_cons(K, KNIL, KNIL);
                        kset_mark(top, new_pair);
                        /* copy the source code info */
                        TValue si = ktry_get_si(K, top);
                        if (!ttisnil(si))
                            kset_source_info(K, new_pair, si);
                    }
                    /* keep the old pair and continue with the car */
                    ks_tbpush(K, ST_CAR); 
                    ks_spush(K, top); 

                    ks_tbpush(K, ST_PUSH); 
                    ks_spush(K, kcar(top)); 
                } else {
                    /* marked pair means a cycle was found */
                    /* NOTE: the pair should be in the stack already so
                       it isn't necessary to push it again to clear the mark */
                    ptree_clear_all(K, sym_ls);
                    klispE_throw_simple(K, "cycle detected in ptree");
                    /* avoid warning */
                    return KNIL;
                }
                break;
            }
            default:
                ptree_clear_all(K, sym_ls);
                klispE_throw_simple(K, "bad object type in ptree");
                /* avoid warning */
                return KNIL;
            }
        } else { 
            /* last operation was a pop */
            /* top is a marked pair, the mark is the copied obj */
            /* NOTE: if top is immutable the mark is also top 
               we could still do the set-car/set-cdr because the
               copy would be the same as the car/cdr, but why bother */
            if (state == ST_CAR) { 
                /* only car was checked (not yet copied) */
                if (kis_mutable(top)) {
                    TValue copied_pair = kget_mark(top);
                    /* copied_pair may be immutable */
                    kset_car_unsafe(K, copied_pair, copy);
                }
                /* put the copied pair again, continue with the cdr */
                ks_tbpush(K, ST_CDR);
                ks_spush(K, top); 

                ks_tbpush(K, ST_PUSH);
                ks_spush(K, kcdr(top)); 
            } else { 
                /* both car & cdr were checked (cdr not yet copied) */
                TValue copied_pair = kget_mark(top);
                /* the unmark is needed to allow diamonds */
                kunmark(top);

                if (kis_mutable(top)) {
                    /* copied_pair may be immutable */
                    kset_cdr_unsafe(K, copied_pair, copy);
                }
                copy = copied_pair;
            }
        }
    }

    if (ttissymbol(penv)) {
        if (kis_symbol_marked(penv)) {
            ptree_clear_all(K, sym_ls);
            klispE_throw_simple_with_irritants(K, "same symbol in both ptree "
                                               "and environment parameter",
                                               1, sym_ls);
        }
    } else if (!ttisignore(penv)) {
	    ptree_clear_all(K, sym_ls);
	    klispE_throw_simple(K, "symbol or #ignore expected as "
                            "environment parmameter");
    }
    ptree_clear_all(K, sym_ls);
    krooted_vars_pop(K);
    return copy;
}

/* Helpers for map (also used by for each) */
void map_for_each_get_metrics(klisp_State *K, TValue lss,
                              int32_t *app_apairs_out, int32_t *app_cpairs_out,
                              int32_t *res_apairs_out, int32_t *res_cpairs_out)
{
    /* avoid warnings (shouldn't happen if _No_return was used in throw) */
    *app_apairs_out = 0;
    *app_cpairs_out = 0;
    *res_apairs_out = 0;
    *res_cpairs_out = 0;

    /* get the metrics of the ptree of each call to app */
    int32_t app_pairs, app_cpairs;
    check_list(K, true, lss, &app_pairs, &app_cpairs);
    int32_t app_apairs = app_pairs - app_cpairs;

    /* get the metrics of the result list */
    int32_t res_pairs, res_cpairs;
    /* We now that lss has at least one elem */
    check_list(K, true, kcar(lss), &res_pairs, &res_cpairs);
    int32_t res_apairs = res_pairs - res_cpairs;
    
    if (res_cpairs == 0) {
        /* finite list of length res_pairs (all lists should
           have the same structure: acyclic with same length) */
        int32_t pairs = app_pairs - 1;
        TValue tail = kcdr(lss);
        while(pairs--) {
            int32_t first_pairs, first_cpairs;
            check_list(K, true, kcar(tail), &first_pairs, &first_cpairs);
            tail = kcdr(tail);

            if (first_cpairs != 0) {
                klispE_throw_simple(K, "mixed finite and infinite lists");
                return;
            } else if (first_pairs != res_pairs) {
                klispE_throw_simple(K, "lists of different length");
                return;
            }
        }
    } else {
        /* cyclic list: all lists should be cyclic.
           result will have acyclic length equal to the
           max of all the lists and cyclic length equal to the lcm
           of all the lists. res_pairs may be broken but will be 
           restored by after the loop */
        int32_t pairs = app_pairs - 1;
        TValue tail = kcdr(lss);
        while(pairs--) {
            int32_t first_pairs, first_cpairs;
            check_list(K, true, kcar(tail), &first_pairs, &first_cpairs);
            int32_t first_apairs = first_pairs - first_cpairs;
            tail = kcdr(tail);

            if (first_cpairs == 0) {
                klispE_throw_simple(K, "mixed finite and infinite lists");
                return;
            } 
            res_apairs = kmax32(res_apairs, first_apairs);
            /* this can throw an error if res_cpairs doesn't 
               fit in 32 bits, which is a reasonable implementation
               restriction because the list wouldn't fit in memory 
               anyways */
            res_cpairs = kcheck32(K, "map/for-each: result list is too big", 
                                  klcm32_64(res_cpairs, first_cpairs));
        }
        res_pairs = kcheck32(K, "map/for-each: result list is too big", 
                             (int64_t) res_cpairs + (int64_t) res_apairs);
        UNUSED(res_pairs);
    }

    *app_apairs_out = app_apairs;
    *app_cpairs_out = app_cpairs;
    *res_apairs_out = res_apairs;
    *res_cpairs_out = res_cpairs;
}

/* Return two lists, isomorphic to lss: one list of cars and one list
   of cdrs (replacing the value of lss) */

/* GC: assumes lss is rooted */
TValue map_for_each_get_cars_cdrs(klisp_State *K, TValue *lss, 
                                  int32_t apairs, int32_t cpairs)
{
    TValue tail = *lss;

    TValue cars = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &cars);
    TValue lp_cars = cars;
    TValue lap_cars = lp_cars;

    TValue cdrs = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &cdrs);
    TValue lp_cdrs = cdrs;
    TValue lap_cdrs = lp_cdrs;
    
    while(apairs != 0 || cpairs != 0) {
        int32_t pairs;
        if (apairs != 0) {
            pairs = apairs;
        } else {
            /* remember last acyclic pair of both lists to to encycle! later */
            lap_cars = lp_cars;
            lap_cdrs = lp_cdrs;
            pairs = cpairs;
        }

        while(pairs--) {
            TValue first = kcar(tail);
            tail = kcdr(tail);
	 
            /* accumulate both cars and cdrs */
            TValue np;
            np = kcons(K, kcar(first), KNIL);
            kset_cdr(lp_cars, np);
            lp_cars = np;

            np = kcons(K, kcdr(first), KNIL);
            kset_cdr(lp_cdrs, np);
            lp_cdrs = np;
        }

        if (apairs != 0) {
            apairs = 0;
        } else {
            cpairs = 0;
            /* encycle! the list of cars and the list of cdrs */
            TValue fcp, lcp;
            fcp = kcdr(lap_cars);
            lcp = lp_cars;
            kset_cdr(lcp, fcp);

            fcp = kcdr(lap_cdrs);
            lcp = lp_cdrs;
            kset_cdr(lcp, fcp);
        }
    }

    krooted_vars_pop(K);
    krooted_vars_pop(K);
    *lss = kcdr(cdrs);
    return kcdr(cars);
}

/* Transpose lss so that the result is a list of lists, each one having
   metrics (app_apairs, app_cpairs). The metrics of the returned list
   should be (res_apairs, res_cpairs) */

/* GC: assumes lss is rooted */
TValue map_for_each_transpose(klisp_State *K, TValue lss, 
                              int32_t app_apairs, int32_t app_cpairs, 
                              int32_t res_apairs, int32_t res_cpairs)
{
    TValue tlist = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &tlist);    
    TValue lp = tlist;
    TValue lap = lp;

    TValue cars = KNIL; /* put something for GC */
    TValue tail = lss;

    /* GC: both cars & tail vary in each loop, to protect them we need
       the vars stack */
    krooted_vars_push(K, &cars);
    krooted_vars_push(K, &tail);
    
    /* Loop over list of lists, creating a list of cars and 
       a list of cdrs, accumulate the list of cars and loop
       with the list of cdrs as the new list of lists (lss) */
    while(res_apairs != 0 || res_cpairs != 0) {
        int32_t pairs;
	
        if (res_apairs != 0) {
            pairs = res_apairs;
        } else {
            pairs = res_cpairs;
            /* remember last acyclic pair to encycle! later */
            lap = lp;
        }

        while(pairs--) {
            /* accumulate cars and replace tail with cdrs */
            cars = map_for_each_get_cars_cdrs(K, &tail, app_apairs, app_cpairs);
            TValue np = kcons(K, cars, KNIL);
            kset_cdr(lp, np);
            lp = np;
        }

        if (res_apairs != 0) {
            res_apairs = 0;
        } else {
            res_cpairs = 0;
            /* encycle! the list of list of cars */
            TValue fcp = kcdr(lap);
            TValue lcp = lp;
            kset_cdr(lcp, fcp);
        }
    }

    krooted_vars_pop(K);
    krooted_vars_pop(K);
    krooted_vars_pop(K);
    return kcdr(tlist);
}

/* Continuations that are used in more than one file */

/* Helper for $sequence, $vau, $lambda, ... */
/* the remaining list can't be null, that case is managed before */
void do_seq(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));

    UNUSED(obj);

    /* 
    ** xparams[0]: remaining list
    ** xparams[1]: dynamic environment
    */
    TValue ls = xparams[0];
    TValue first = kcar(ls);
    TValue tail = kcdr(ls);
    TValue denv = xparams[1];

    if (ttispair(tail)) {
        TValue new_cont = kmake_continuation(K, kget_cc(K), do_seq, 2, tail, 
                                             denv);
        kset_cc(K, new_cont);
#if KTRACK_SI
        /* put the source info of the list including the element
           that we are about to evaluate */
        kset_source_info(K, new_cont, ktry_get_si(K, ls));
#endif
    }
    ktail_eval(K, first, denv);
}

/* this is used for inner & outer continuations, it just
   passes the value. xparams is not actually empty, it contains
   the entry/exit guards, but they are used only in 
   continuation->applicative (that is during abnormal passes) */
void do_pass_value(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    UNUSED(xparams);
    kapply_cc(K, obj);
}

/* 
** Continuation that ignores the value received and instead returns
** a previously computed value.
*/
void do_return_value(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: saved_obj
    */
    UNUSED(obj);
    TValue ret_obj = xparams[0];
    kapply_cc(K, ret_obj);
}

/* binder returned */
void do_bind(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: dynamic key 
    */
    bind_2tp(K, ptree, "any", anytype, obj,
	         "combiner", ttiscombiner, comb);
    UNUSED(denv); /* the combiner is called in an empty environment */
    TValue key = xparams[0];
    /* GC: root intermediate objs */
    TValue new_flag = KTRUE;
    TValue new_value = obj;
    TValue old_flag = kcar(key);
    TValue old_value = kcdr(key);
    /* set the var to the new object */
    kset_car(key, new_flag);
    kset_cdr(key, new_value);
    /* Old value must be protected from GC. It is no longer
       reachable through key and not yet reachable through
       continuation xparams. Boolean flag needn't be rooted,
       because is not heap-allocated. */
    krooted_tvs_push(K, old_value);
    /* create a continuation to set the var to the correct value/flag on both
       normal return and abnormal passes */
    TValue new_cont = make_bind_continuation(K, key, old_flag, old_value,
                                             new_flag, new_value);
    krooted_tvs_pop(K);
    kset_cc(K, new_cont); /* implicit rooting */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue expr = kcons(K, comb, KNIL);
    krooted_tvs_pop(K);
    ktail_eval(K, expr, env)
        }

/* accesor returned */
void do_access(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: dynamic key 
    */
    check_0p(K, ptree);
    UNUSED(denv);
    TValue key = xparams[0];

    if (kis_true(kcar(key))) {
        kapply_cc(K, kcdr(key));
    } else {
        klispE_throw_simple(K, "variable is unbound");
        return;
    }
}

/* continuation to set the key to the old value on normal return */
void do_unbind(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /*
    ** xparams[0]: dynamic key
    ** xparams[1]: old flag
    ** xparams[2]: old value
    */

    TValue key = xparams[0];
    TValue old_flag = xparams[1];
    TValue old_value = xparams[2];

    kset_car(key, old_flag);
    kset_cdr(key, old_value);
    /* pass along the value returned to this continuation */
    kapply_cc(K, obj);
}

/* operative for setting the key to the new/old flag/value */
void do_set_pass(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    /*
    ** xparams[0]: dynamic key
    ** xparams[1]: flag
    ** xparams[2]: value
    */
    TValue key = xparams[0];
    TValue flag = xparams[1];
    TValue value = xparams[2];
    UNUSED(denv);

    kset_car(key, flag);
    kset_cdr(key, value);

    /* pass to next interceptor/ final destination */
    /* ptree is as for interceptors: (obj divert) */
    TValue obj = kcar(ptree);
    kapply_cc(K, obj);
}

/* /Continuations that are used in more than one file */

/* dynamic keys */
/* create continuation to set the key on both normal return and
   abnormal passes */
/* TODO: reuse the code for guards in kgcontinuations.c */

/* GC: this assumes that key, old_value and new_value are rooted */
TValue make_bind_continuation(klisp_State *K, TValue key,
                              TValue old_flag, TValue old_value, 
                              TValue new_flag, TValue new_value)
{
    TValue unbind_cont = kmake_continuation(K, kget_cc(K), 
                                            do_unbind, 3, key, old_flag, 
                                            old_value);
    krooted_tvs_push(K, unbind_cont);
    /* create the guards to guarantee that the values remain consistent on
       abnormal passes (in both directions) */
    TValue exit_int = kmake_operative(K, do_set_pass, 
                                      3, key, old_flag, old_value);
    krooted_tvs_push(K, exit_int);
    TValue exit_guard = kcons(K, K->root_cont, exit_int);
    krooted_tvs_pop(K); /* already rooted in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* already rooted in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_int = kmake_operative(K, do_set_pass, 
                                       3, key, new_flag, new_value);
    krooted_tvs_push(K, entry_int);
    TValue entry_guard = kcons(K, K->root_cont, entry_int);
    krooted_tvs_pop(K); /* already rooted in guard */
    krooted_tvs_push(K, entry_guard);
    TValue entry_guards = kcons(K, entry_guard, KNIL);
    krooted_tvs_pop(K); /* already rooted in guards */
    krooted_tvs_push(K, entry_guards);


    /* NOTE: in the stack now we have the unbind cont & two guard lists */
    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, unbind_cont, 
                                           do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
                                           do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);

    /* unbind_cont & 2 guard_lists */
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);
    /* env & outer_cont */
    krooted_tvs_pop(K); krooted_tvs_pop(K);
    
    return inner_cont;
}

/* Helpers for guard-continuation (& guard-dynamic-extent) */

#define singly_wrapped(obj_) (ttisapplicative(obj_) &&      \
                              ttisoperative(kunwrap(obj_)))

/* this unmarks root before throwing any error */
/* TODO: this isn't very clean, refactor */

/* GC: assumes obj & root are rooted */
static inline TValue check_copy_single_entry(klisp_State *K, char *name,
                                      TValue obj, TValue root)
{
    if (!ttispair(obj) || !ttispair(kcdr(obj)) || 
	    !ttisnil(kcddr(obj))) {
        unmark_list(K, root);
        klispE_throw_simple(K, "Bad entry (expected list of length 2)");
        return KINERT;
    } 
    TValue cont = kcar(obj);
    TValue app = kcadr(obj);

    if (!ttiscontinuation(cont)) {
        unmark_list(K, root);
        klispE_throw_simple(K, "Bad type on first element (expected " 
                            "continuation)");				        
        return KINERT;
    } else if (!singly_wrapped(app)) { 
        unmark_list(K, root);
        klispE_throw_simple(K, "Bad type on second element (expected " 
                            "singly wrapped applicative)");				        
        return KINERT; 
    }

    /* save the operative directly, don't waste space/time
       with a list, use just a pair */
    return kcons(K, cont, kunwrap(app)); 
}

/* the guards are probably generated on the spot so we don't check
   for immutability and copy it anyways */
/* GC: Assumes obj is rooted */
TValue check_copy_guards(klisp_State *K, char *name, TValue obj)
{
    if (ttisnil(obj)) {
        return obj;
    } else {
        TValue copy = kcons(K, KNIL, KNIL);
        krooted_vars_push(K, &copy);
        TValue last_pair = copy;
        TValue tail = obj;
    
        while(ttispair(tail) && !kis_marked(tail)) {
            /* this will clear the marks and throw an error if the structure
               is incorrect */
            TValue entry = check_copy_single_entry(K, name, kcar(tail), obj);
            krooted_tvs_push(K, entry);
            TValue new_pair = kcons(K, entry, KNIL);
            krooted_tvs_pop(K);
            kmark(tail);
            kset_cdr(last_pair, new_pair);
            last_pair = new_pair;
            tail = kcdr(tail);
        }

        /* dont close the cycle (if there is one) */
        unmark_list(K, obj);
        if (!ttispair(tail) && !ttisnil(tail)) {
            klispE_throw_simple(K, "expected list"); 
            return KINERT;
        } 
        krooted_vars_pop(K);
        return kcdr(copy);
    }
}

void guard_dynamic_extent(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);

    bind_3tp(K, ptree, "any", anytype, entry_guards,
             "combiner", ttiscombiner, comb,
             "any", anytype, exit_guards);

    entry_guards = check_copy_guards(K, "guard-dynamic-extent: entry guards", 
                                     entry_guards);
    krooted_tvs_push(K, entry_guards);
    exit_guards = check_copy_guards(K, "guard-dynamic-extent: exit guards", 
                                    exit_guards);
    krooted_tvs_push(K, exit_guards);
    /* GC: root continuations */
    /* The current continuation is guarded */
    TValue outer_cont = kmake_continuation(K, kget_cc(K), do_pass_value, 
                                           2, entry_guards, denv);
    kset_outer_cont(outer_cont);
    kset_cc(K, outer_cont); /* this implicitly roots outer_cont */

    TValue inner_cont = kmake_continuation(K, outer_cont, do_pass_value, 2, 
                                           exit_guards, denv);
    kset_inner_cont(inner_cont);

    /* call combiner with no operands in the dynamic extent of inner,
       with the dynamic env of this call */
    kset_cc(K, inner_cont); /* this implicitly roots inner_cont */
    TValue expr = kcons(K, comb, KNIL);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ktail_eval(K, expr, denv);
}
