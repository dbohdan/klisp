/*
** kghelpers.h
** Helper macros and functions for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kghelpers_h
#define kghelpers_h

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kerror.h"
#include "kpair.h"
#include "kvector.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kenvironment.h"
#include "ksymbol.h"
#include "kstring.h"
#include "ktable.h"

/* 
** REFACTOR split this file into several.
** Some should have their own files (like knumber, kbool, etc)
** Others are simply helpers that should be split into modules
** (like continuation helpers, list helpers, environment helpers)
*/
   
/* Initialization of continuation names */
void kinit_kghelpers_cont_names(klisp_State *K);

/* to use in type checking binds when no check is needed */
#define anytype(obj_) (true)

/* Type predicates */
/* TODO these should be moved to either kobject.h or the corresponding
   files (e.g. kbooleanp to kboolean.h */
bool kbooleanp(TValue obj);
bool kcombinerp(TValue obj);
bool knumberp(TValue obj);
bool knumber_wpvp(TValue obj);
bool kfinitep(TValue obj);
bool kintegerp(TValue obj);
bool keintegerp(TValue obj);
bool krationalp(TValue obj);
bool krealp(TValue obj);
bool kreal_wpvp(TValue obj);
bool kexactp(TValue obj);
bool kinexactp(TValue obj);
bool kundefinedp(TValue obj);
bool krobustp(TValue obj);
bool ku8p(TValue obj);
/* This is used in gcd & lcm */
bool kimp_intp(TValue obj);

/* needed by kgffi.c and encapsulations */
void enc_typep(klisp_State *K);

/* /Type predicates */

/* some number predicates */
/* REFACTOR: These should be in a knumber.h header */

/* Misc Helpers */
/* TEMP: only reals (no complex numbers) */
bool kpositivep(TValue n);
bool knegativep(TValue n);

static inline bool kfast_zerop(TValue n) 
{ 
    return (ttisfixint(n) && ivalue(n) == 0) ||
        (ttisdouble(n) && dvalue(n) == 0.0); 
}

static inline bool kfast_onep(TValue n) 
{ 
    return (ttisfixint(n) && ivalue(n) == 1) ||
        (ttisdouble(n) && dvalue(n) == 1.0); 
}

static inline TValue kneg_inf(TValue i) 
{ 
    if (ttiseinf(i))
        return tv_equal(i, KEPINF)? KEMINF : KEPINF; 
    else /* ttisiinf(i) */
        return tv_equal(i, KIPINF)? KIMINF : KIPINF; 
}

static inline bool knum_same_signp(klisp_State *K, TValue n1, TValue n2) 
{ 
    return kpositivep(n1) == kpositivep(n2); 
}

/* /some number predicates */

/*
** NOTE: these are intended to be used at the beginning of a function
**   they expand to more than one statement and may evaluate some of
**   their arguments more than once 
*/

/* XXX: add parens around macro vars!! */
/* TODO try to rewrite all of these with just check_0p and check_al1p,
   (the same with check_0tp and check_al1tp)
   add a number param and use an array of strings for msgs */

#define check_0p(K_, ptree_)                                        \
    if (!ttisnil(ptree_)) {                                         \
        klispE_throw_simple((K_),                                   \
                            "Bad ptree (expected no arguments)");   \
        return;                                                     \
    }

#define bind_1p(K_, ptree_, v_)                     \
    bind_1tp((K_), (ptree_), "any", anytype, (v_))

#define bind_1tp(K_, ptree_, tstr_, t_, v_)                         \
    TValue v_;                                                      \
    if (!ttispair(ptree_) || !ttisnil(kcdr(ptree_))) {              \
        klispE_throw_simple((K_),                                   \
                            "Bad ptree (expected one argument)");   \
        return;                                                     \
    }                                                               \
    v_ = kcar(ptree_);                                              \
    if (!t_(v_)) {                                                  \
        klispE_throw_simple(K_, "Bad type on first argument "       \
                            "(expected "	tstr_ ")");             \
        return;                                                     \
    } 


#define bind_2p(K_, ptree_, v1_, v2_)               \
    bind_2tp((K_), (ptree_), "any", anytype, (v1_), \
             "any", anytype, (v2_))

#define bind_2tp(K_, ptree_, tstr1_, t1_, v1_,                          \
                 tstr2_, t2_, v2_)                                      \
    TValue v1_, v2_;                                                    \
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_)) ||                 \
	    !ttisnil(kcddr(ptree_))) {                                      \
        klispE_throw_simple(K_, "Bad ptree (expected two arguments)");  \
        return;                                                         \
    }                                                                   \
    v1_ = kcar(ptree_);                                                 \
    v2_ = kcadr(ptree_);                                                \
    if (!t1_(v1_)) {                                                    \
        klispE_throw_simple(K_, "Bad type on first argument (expected " \
                            tstr1_ ")");                                \
        return;                                                         \
    } else if (!t2_(v2_)) {                                             \
        klispE_throw_simple(K_, "Bad type on second argument (expected " \
                            tstr2_ ")");                                \
        return;                                                         \
    }

#define bind_3p(K_, ptree_, v1_, v2_, v3_)              \
    bind_3tp(K_, ptree_, "any", anytype, v1_,           \
             "any", anytype, v2_, "any", anytype, v3_)

#define bind_3tp(K_, ptree_, tstr1_, t1_, v1_,                          \
                 tstr2_, t2_, v2_, tstr3_, t3_, v3_)                    \
    TValue v1_, v2_, v3_;                                               \
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_)) ||                 \
        !ttispair(kcddr (ptree_)) || !ttisnil(kcdddr(ptree_))) {        \
        klispE_throw_simple(K_, "Bad ptree (expected three arguments)"); \
        return;                                                         \
    }                                                                   \
    v1_ = kcar(ptree_);                                                 \
    v2_ = kcadr(ptree_);                                                \
    v3_ = kcaddr(ptree_);                                               \
    if (!t1_(v1_)) {                                                    \
        klispE_throw_simple(K_, "Bad type on first argument (expected " \
                            tstr1_ ")");                                \
        return;                                                         \
    } else if (!t2_(v2_)) {                                             \
        klispE_throw_simple(K_, "Bad type on second argument (expected " \
                            tstr2_ ")");                                \
        return;                                                         \
    } else if (!t3_(v3_)) {                                             \
        klispE_throw_simple(K_, "Bad type on third argument (expected " \
                            tstr3_ ")");                                \
        return;                                                         \
    }

/* bind at least 1 parameter, like (v1_ . v2_) */
#define bind_al1p(K_, ptree_, v1_, v2_)                         \
    bind_al1tp((K_), (ptree_), "any", anytype, (v1_), (v2_))

/* bind at least 1 parameters (with type), like (v1_ . v2_) */
#define bind_al1tp(K_, ptree_, tstr1_, t1_, v1_, v2_)                   \
    TValue v1_, v2_;                                                    \
    if (!ttispair(ptree_)) {                                            \
        klispE_throw_simple(K_, "Bad ptree (expected at least "         \
                            "one argument)");                           \
        return;                                                         \
    }                                                                   \
    v1_ = kcar(ptree_);                                                 \
    v2_ = kcdr(ptree_);                                                 \
    if (!t1_(v1_)) {                                                    \
        klispE_throw_simple(K_, "Bad type on first argument (expected " \
                            tstr1_ ")");                                \
        return;                                                         \
    }

/* bind at least 2 parameters, like (v1_ v2_ . v3_) */
#define bind_al2p(K_, ptree_, v1_, v2_, v3_)            \
    bind_al2tp((K_), (ptree_), "any", anytype, (v1_),	\
               "any", anytype, (v2_), (v3_))				

/* bind at least 2 parameters (with type), like (v1_ v2_ . v3_) */
#define bind_al2tp(K_, ptree_, tstr1_, t1_, v1_,                        \
                   tstr2_, t2_, v2_, v3_)                               \
    TValue v1_, v2_, v3_;                                               \
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_))) {                 \
        klispE_throw_simple(K_, "Bad ptree (expected at least "         \
                            "two arguments)");                          \
        return;                                                         \
    }                                                                   \
    v1_ = kcar(ptree_);                                                 \
    v2_ = kcadr(ptree_);                                                \
    v3_ = kcddr(ptree_);                                                \
    if (!t1_(v1_)) {                                                    \
        klispE_throw_simple(K_, "Bad type on first argument (expected " \
                            tstr1_ ")");                                \
        return;                                                         \
    } else if (!t2_(v2_)) {                                             \
        klispE_throw_simple(K_, "Bad type on second argument (expected " \
                            tstr2_ ")");                                \
        return;                                                         \
    }

/* bind at least 3 parameters, like (v1_ v2_ v3_ . v4_) */
#define bind_al3p(K_, ptree_, v1_, v2_, v3_, v4_)                   \
    bind_al3tp((K_), (ptree_), "any", anytype, (v1_),               \
               "any", anytype, (v2_), "any", anytype, (v3_), (v4_)) \

/* bind at least 3 parameters (with type), like (v1_ v2_ v3_ . v4_) */
#define bind_al3tp(K_, ptree_, tstr1_, t1_, v1_,                        \
                   tstr2_, t2_, v2_, tstr3_, t3_, v3_, v4_)             \
    TValue v1_, v2_, v3_, v4_;                                          \
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_)) ||                 \
        !ttispair(kcddr(ptree_))) {                                     \
        klispE_throw_simple(K_, "Bad ptree (expected at least "         \
                            "three arguments)");                        \
        return;                                                         \
    }                                                                   \
    v1_ = kcar(ptree_);                                                 \
    v2_ = kcadr(ptree_);                                                \
    v3_ = kcaddr(ptree_);                                               \
    v4_ = kcdddr(ptree_);                                               \
    if (!t1_(v1_)) {                                                    \
        klispE_throw_simple(K_, "Bad type on first argument (expected " \
                            tstr1_ ")");                                \
        return;                                                         \
    } else if (!t2_(v2_)) {                                             \
        klispE_throw_simple(K_, "Bad type on second argument (expected " \
                            tstr2_ ")");                                \
        return;                                                         \
    } else if (!t3_(v3_)) {                                             \
        klispE_throw_simple(K_, "Bad type on third argument (expected " \
                            tstr3_ ")");                                \
        return;                                                         \
    }


/* returns true if the obj pointed by par is a list of one element of 
   type type, and puts that element in par
   returns false if par is nil
   In any other case it throws an error */
#define get_opt_tpar(K_, par_, tstr_, t_)  ({                           \
            bool res_;                                                  \
            if (ttisnil(par_)) {                                        \
                res_ = false;                                           \
            } else if (!ttispair(par_) || !ttisnil(kcdr(par_))) {		\
                klispE_throw_simple((K_),                               \
                                    "Bad ptree structure "              \
                                    "(in optional argument)");			\
                return;                                                 \
            } else if (!t_(kcar(par_))) {                               \
                klispE_throw_simple(K_, "Bad type on optional argument " \
                                    "(expected "	tstr_ ")");         \
                return;                                                 \
            } else {                                                    \
                par_ = kcar(par_);                                      \
                res_ = true;                                            \
            }                                                           \
            res_; })								

/*
** This states are useful for traversing trees, saving the state in the
** token char buffer
*/
#define ST_PUSH ((char) 0)
#define ST_CAR ((char) 1)
#define ST_CDR ((char) 2)

/*
** Unmarking structures. 
** MAYBE: These shouldn't be inline really.
** These two stop at the first object that is not a marked pair
*/
static inline void unmark_list(klisp_State *K, TValue obj)
{
    UNUSED(K); /* not needed, it's here for consistency */
    while(ttispair(obj) && kis_marked(obj)) {
        kunmark(obj);
        obj = kcdr(obj);
    }
}

static inline void unmark_tree(klisp_State *K, TValue obj)
{
    assert(ks_sisempty(K));

    ks_spush(K, obj);

    while(!ks_sisempty(K)) {
        obj = ks_spop(K);

        if (ttispair(obj) && kis_marked(obj)) {
            kunmark(obj);
            ks_spush(K, kcdr(obj));
            ks_spush(K, kcar(obj));
        } else if (ttisvector(obj) && kis_marked(obj)) {
            kunmark(obj);
            uint32_t i = kvector_size(obj);
            const TValue *array = kvector_buf(obj);
            while(i-- > 0)
                ks_spush(K, array[i]);
        }
    }
}

/*
** Structure checking and copying
*/

/* TODO: move all bools to a flag parameter (with constants like
   KCHK_LS_FORCE_COPY, KCHK_ALLOW_CYCLE, KCHK_AVOID_ENCYCLE, etc) */
/* typed finite list. Structure error are thrown before type errors */
void check_typed_list(klisp_State *K, bool (*typep)(TValue), bool allow_infp, 
                      TValue obj, int32_t *pairs, int32_t *cpairs);

/* check that obj is a list, returns the number of pairs */
/* TODO change the return to void and add int32_t pairs obj */
void check_list(klisp_State *K, bool allow_infp, TValue obj, 
                int32_t *pairs, int32_t *cpairs);

/* TODO: add unchecked_copy_list */
/* TODO: add check_copy_typed_list */
/* check that obj is a list and make a copy if it is not immutable or
   force_copy is true */
/* GC: assumes obj is rooted */
TValue check_copy_list(klisp_State *K, TValue obj, bool force_copy, 
                       int32_t *pairs, int32_t *cpairs);

/* check that obj is a list of environments and make a copy but don't keep 
   the cycles */
/* GC: assume obj is rooted */
TValue check_copy_env_list(klisp_State *K, TValue obj);

/* The assimetry in error checking in the following functions
   is a product of the contexts in which they are used, see the
   .c for an enumeration of such contexts */
/* list->? conversion functions, only type errors of elems checked */
TValue list_to_string_h(klisp_State *K, TValue ls, int32_t length);
TValue list_to_vector_h(klisp_State *K, TValue ls, int32_t length);
TValue list_to_bytevector_h(klisp_State *K, TValue ls, int32_t length);

/* ?->list conversion functions, type checked */
TValue string_to_list_h(klisp_State *K, TValue obj, int32_t *length);
TValue vector_to_list_h(klisp_State *K, TValue obj, int32_t *length);
TValue bytevector_to_list_h(klisp_State *K, TValue obj, int32_t *length);

/*
** Generic function for type predicates
** It can only be used by types that have a unique tag
*/
void typep(klisp_State *K);

/*
** Generic function for type predicates
** It takes an arbitrary function pointer of type bool (*fn)(TValue o)
*/
void ftypep(klisp_State *K);

/*
** Generic function for typed predicates (like char-alphabetic? or finite?)
** A typed predicate is a predicate that requires its arguments to be a certain
** type. This takes a function pointer for the type & one for the predicate,
** both of the same type: bool (*fn)(TValue o).
** On zero operands this return true
*/
void ftyped_predp(klisp_State *K);

/*
** Generic function for typed binary predicates (like =? & char<?)
** A typed predicate is a predicate that requires its arguments to be a certain
** type. This takes a function pointer for the type bool (*typep)(TValue o) 
** & one for the predicate: bool (*fn)(TValue o1, TValue o2).
** This assumes the predicate is transitive and works even in cyclic lists
** On zero and one operand this return true
*/
void ftyped_bpredp(klisp_State *K);

/* This is the same, but the comparison predicate takes a klisp_State */
/* TODO unify them */
void ftyped_kbpredp(klisp_State *K);

/* Continuations that are used in more than one file */
void do_seq(klisp_State *K);
void do_pass_value(klisp_State *K);
void do_return_value(klisp_State *K);
void do_bind(klisp_State *K);
void do_access(klisp_State *K);
void do_unbind(klisp_State *K);
void do_set_pass(klisp_State *K);
/* /Continuations that are used in more than one file */

/* dynamic var */
TValue make_bind_continuation(klisp_State *K, TValue key,
                              TValue old_flag, TValue old_value, 
                              TValue new_flag, TValue new_value);

TValue check_copy_guards(klisp_State *K, char *name, TValue obj);
void guard_dynamic_extent(klisp_State *K);

/* Some helpers for working with fixints (signed 32 bits) */
static inline int32_t kabs32(int32_t a) { return a < 0? -a : a; }
static inline int64_t kabs64(int64_t a) { return a < 0? -a : a; }
static inline int32_t kmin32(int32_t a, int32_t b) { return a < b? a : b; }
static inline int32_t kmax32(int32_t a, int32_t b) { return a > b? a : b; }

static inline int32_t kcheck32(klisp_State *K, char *msg, int64_t i) 
{
    if (i > (int64_t) INT32_MAX || i < (int64_t) INT32_MIN) {
        klispE_throw_simple(K, msg);
        return 0;
    } else {
        return (int32_t) i;
    }
}

/* gcd for two numbers, used for gcd, lcm & map */
int64_t kgcd32_64(int32_t a, int32_t b);
int64_t klcm32_64(int32_t a, int32_t b);

/*
** Other
*/

/* memoize applicative (used in kstate & promises) */
void memoize(klisp_State *K);
/* list applicative (used in kstate and kgpairs_lists) */
void list(klisp_State *K);

/* Helper for list-tail, list-ref and list-set! */
int32_t ksmallest_index(klisp_State *K, TValue obj, TValue tk);

/* Helper for get-list-metrics, and list-tail, list-ref and list-set! 
   when receiving bigint indexes */
void get_list_metrics_aux(klisp_State *K, TValue obj, int32_t *p, int32_t *n, 
                          int32_t *a, int32_t *c);

/* Helper for eq? and equal? */
bool eq2p(klisp_State *K, TValue obj1, TValue obj2);

/* Helper for equal?, assoc and member */
/* compare two objects and check to see if they are "equal?". */
bool equal2p(klisp_State *K, TValue obj1, TValue obj2);

/* Helper (also used by $vau, $lambda, etc) */
TValue copy_es_immutable_h(klisp_State *K, TValue ptree, bool mut_flag);

/* ptree handling */
void match(klisp_State *K, TValue env, TValue ptree, TValue obj);
TValue check_copy_ptree(klisp_State *K, TValue ptree, TValue penv);

/* map/$for-each */
/* Helpers for map (also used by for-each) */

/* Calculate the metrics for both the result list and the ptree
   passed to the applicative */
void map_for_each_get_metrics(
    klisp_State *K, TValue lss, int32_t *app_apairs_out, 
    int32_t *app_cpairs_out, int32_t *res_apairs_out, int32_t *res_cpairs_out);

/* Return two lists, isomorphic to lss: one list of cars and one list
   of cdrs (replacing the value of lss) */
/* GC: Assumes lss is rooted */
TValue map_for_each_get_cars_cdrs(klisp_State *K, TValue *lss, 
                                  int32_t apairs, int32_t cpairs);

/* Transpose lss so that the result is a list of lists, each one having
   metrics (app_apairs, app_cpairs). The metrics of the returned list
   should be (res_apairs, res_cpairs) */

/* GC: Assumes lss is rooted */
TValue map_for_each_transpose(klisp_State *K, TValue lss, 
                              int32_t app_apairs, int32_t app_cpairs, 
                              int32_t res_apairs, int32_t res_cpairs);


/*
** Macros for ground environment initialization
*/

/*
** BEWARE: this is highly unhygienic, it assumes variables "symbol" and
** "value", both of type TValue. symbol will be bound to a symbol named by
** "n_" and can be referrenced in the var_args
** GC: All of these should be called when GC is deactivated
*/

/* TODO add si to the symbols */
#if KTRACK_SI
#define add_operative(K_, env_, n_, fn_, ...)               \
    { symbol = ksymbol_new_b(K_, n_, KNIL);                 \
        value = kmake_operative(K_, fn_, __VA_ARGS__);      \
        TValue str = kstring_new_b_imm(K_, __FILE__);       \
        TValue si = kcons(K, str, kcons(K_, i2tv(__LINE__),	\
                                        i2tv(0)));          \
        kset_source_info(K_, value, si);                    \
        kadd_binding(K_, env_, symbol, value); }

#define add_applicative(K_, env_, n_, fn_, ...)				\
    { symbol = ksymbol_new_b(K_, n_, KNIL);                 \
        value = kmake_applicative(K_, fn_, __VA_ARGS__);    \
        TValue str = kstring_new_b_imm(K_, __FILE__);       \
        TValue si = kcons(K, str, kcons(K_, i2tv(__LINE__), \
                                        i2tv(0)));			\
        kset_source_info(K_, kunwrap(value), si);			\
        kset_source_info(K_, value, si);                    \
        kadd_binding(K_, env_, symbol, value); }
#else /* KTRACK_SI */
#define add_operative(K_, env_, n_, fn_, ...)           \
    { symbol = ksymbol_new_b(K_, n_, KNIL);             \
        value = kmake_operative(K_, fn_, __VA_ARGS__);	\
        kadd_binding(K_, env_, symbol, value); }

#define add_applicative(K_, env_, n_, fn_, ...)             \
    { symbol = ksymbol_new_b(K_, n_, KNIL);                 \
        value = kmake_applicative(K_, fn_, __VA_ARGS__);	\
        kadd_binding(K_, env_, symbol, value); }
#endif /* KTRACK_SI */

#define add_value(K_, env_, n_, v_)             \
    { value = v_;                               \
        symbol = ksymbol_new_b(K_, n_, KNIL);   \
        kadd_binding(K_, env_, symbol, v_); }

#endif

/* for initiliazing continuation names */
#define add_cont_name(K_, t_, c_, n_)					\
    { TValue str = kstring_new_b_imm(K_, n_);           \
        TValue *node = klispH_set(K_, t_, p2tv(c_));    \
        *node = str;                                    \
    }

