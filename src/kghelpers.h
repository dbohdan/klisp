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
#include "kapplicative.h"
#include "koperative.h"

/* to use in type checking binds when no check is needed */
#define anytype(obj_) (true)

/*
** NOTE: these are intended to be used at the beginning of a function
**   they expand to more than one statement and may evaluate some of
**   their arguments more than once 
*/

/* XXX: add parens around macro vars!! */
#define check_0p(K_, n_, ptree_) \
    if (!ttisnil(ptree_)) { \
	klispE_throw_extra((K_), (n_) , \
			   ": Bad ptree (expected no arguments)");  \
	return; \
    }

#define bind_1p(K_, n_, ptree_, v_) \
    bind_1tp((K_), (n_), (ptree_), "any", anytype, (v_))

#define bind_1tp(K_, n_, ptree_, tstr_, t_, v_)	\
    TValue v_; \
    if (!ttispair(ptree_) || !ttisnil(kcdr(ptree_))) { \
	klispE_throw_extra((K_), (n_) , \
			   ": Bad ptree (expected one argument)");  \
	return; \
    } \
    v_ = kcar(ptree_);				\
    if (!t_(v_)) { \
	klispE_throw_extra(K_, n_ , ": Bad type on first argument " \
			   "(expected "	tstr_ ")");     \
	return; \
    } 


#define bind_2p(K_, n_, ptree_, v1_, v2_)		\
    bind_2tp((K_), (n_), (ptree_), "any", anytype, (v1_), \
	     "any", anytype, (v2_))

#define bind_2tp(K_, n_, ptree_, tstr1_, t1_, v1_, \
		 tstr2_, t2_, v2_)			\
    TValue v1_, v2_;					\
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_)) || \
	    !ttisnil(kcddr(ptree_))) {		\
	klispE_throw_extra(K_, n_ , ": Bad ptree (expected two arguments)"); \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcadr(ptree_); \
    if (!t1_(v1_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on first argument (expected " \
		     tstr1_ ")");				     \
	return; \
    } else if (!t2_(v2_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on second argument (expected " \
		     tstr2_ ")");				     \
	return; \
    }

#define bind_3p(K_, n_, ptree_, v1_, v2_, v3_)	\
    bind_3tp(K_, n_, ptree_, "any", anytype, v1_, \
	     "any", anytype, v2_, "any", anytype, v3_)

#define bind_3tp(K_, n_, ptree_, tstr1_, t1_, v1_, \
		 tstr2_, t2_, v2_, tstr3_, t3_, v3_)	\
    TValue v1_, v2_, v3_;				\
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_)) ||	    \
	  !ttispair(kcddr (ptree_)) || !ttisnil(kcdddr(ptree_))) {  \
	klispE_throw_extra(K_, n_, ": Bad ptree (expected three arguments)"); \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcadr(ptree_); \
    v3_ = kcaddr(ptree_); \
    if (!t1_(v1_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on first argument (expected " \
		     tstr1_ ")");				     \
	return; \
    } else if (!t2_(v2_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on second argument (expected " \
		     tstr2_ ")");				     \
	return; \
    } else if (!t3_(v3_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on third argument (expected " \
		     tstr3_ ")");				     \
	return; \
    }

/* bind at least 1 parameter, like (v1_ . v2_) */
#define bind_al1p(K_, n_, ptree_, v1_, v2_)		\
    bind_al1tp((K_), (n_), (ptree_), "any", anytype, (v1_), (v2_))

/* bind at least 1 parameters (with type), like (v1_ . v2_) */
#define bind_al1tp(K_, n_, ptree_, tstr1_, t1_, v1_, v2_)	\
    TValue v1_, v2_;				\
    if (!ttispair(ptree_)) {			\
	klispE_throw_extra(K_, n_ , ": Bad ptree (expected at least " \
			   "one argument)");			      \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcdr(ptree_); \
    if (!t1_(v1_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on first argument (expected " \
		     tstr1_ ")");				     \
	return; \
    }

/* bind at least 2 parameters, like (v1_ v2_ . v3_) */
#define bind_al2p(K_, n_, ptree_, v1_, v2_, v3_)		\
    bind_al2tp((K_), (n_), (ptree_), "any", anytype, (v1_),	\
	       "any", anytype, (v2_), (v3_))				

/* bind at least 2 parameters (with type), like (v1_ v2_ . v3_) */
#define bind_al2tp(K_, n_, ptree_, tstr1_, t1_, v1_, \
		   tstr2_, t2_, v2_, v3_)			\
    TValue v1_, v2_, v3_;					\
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_))) {			\
	klispE_throw_extra(K_, n_ , ": Bad ptree (expected at least " \
			   "two arguments)");			      \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcadr(ptree_); \
    v3_ = kcddr(ptree_); \
    if (!t1_(v1_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on first argument (expected " \
		     tstr1_ ")");				     \
	return; \
    } else if (!t2_(v2_)) { \
	klispE_throw_extra(K_, n_, ": Bad type on second argument (expected " \
		     tstr2_ ")");				     \
	return; \
    }


/* returns true if the obj pointed by par is a list of one element of 
   type type, and puts that element in par
   returns false if *par is nil
   In any other case it throws an error */
inline bool get_opt_tpar(klisp_State *K, char *name, int32_t type, TValue *par)
{
    if (ttisnil(*par)) {
	return false;
    } else if (ttispair(*par) && ttisnil(kcdr(*par))) {
	*par = kcar(*par);
	if (ttype(*par) != type) {
	    /* TODO show expected type */
	    klispE_throw_extra(K, name, ": Bad type on optional argument "
			 "(expected ?)");    
	    /* avoid warning */
	    return false;
	} else {
	    return true;
	}
    } else {
	klispE_throw(K, "apply: Bad ptree structure (in optional argument)");
	/* avoid warning */
	return false;
    }
}


/* TODO: add name and source info */
#define make_operative(K_, fn_, ...) \
    kmake_operative(K_, KNIL, KNIL, fn_, __VA_ARGS__)
#define make_applicative(K_, fn_, ...) \
    kwrap(K_, kmake_operative(K_, KNIL, KNIL, fn_, __VA_ARGS__))

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
inline void unmark_list(klisp_State *K, TValue obj)
{
    (void) K; /* not needed, it's here for consistency */
    while(ttispair(obj) && kis_marked(obj)) {
	kunmark(obj);
	obj = kcdr(obj);
    }
}

inline void unmark_tree(klisp_State *K, TValue obj)
{
    assert(ks_sisempty(K));

    ks_spush(K, obj);

    while(!ks_sisempty(K)) {
	obj = ks_spop(K);

	if (ttispair(obj) && kis_marked(obj)) {
	    kunmark(obj);
	    ks_spush(K, kcdr(obj));
	    ks_spush(K, kcar(obj));
	}
    }
}

/*
** Structure checking and copying
*/

/* typed finite list. Structure error should be throw before type errors */
int32_t check_typed_list(klisp_State *K, char *name, char *typename,
			 bool (*typep)(TValue), bool allow_infp, TValue obj,
			 int32_t *cpairs);

/*
** MAYBE: These shouldn't be inline really.
*/

/* check that obj is a list, returns the number of pairs */
inline int32_t check_list(klisp_State *K, char *name, TValue obj)
{
    TValue tail = obj;
    int pairs = 0;
    while(ttispair(tail) && !kis_marked(tail)) {
	kmark(tail);
	tail = kcdr(tail);
	++pairs;
    }
    unmark_list(K, obj);

    if (!ttispair(tail) && !ttisnil(tail)) {
	klispE_throw_extra(K, name , ": expected list"); 
	return 0;
    }
    return pairs;
}

/* check that obj is a list and make a copy if it is not immutable */
inline TValue check_copy_list(klisp_State *K, char *name, TValue obj)
{
    if (ttisnil(obj))
	return obj;

    if (ttispair(obj) && kis_immutable(obj)) {
	(void)check_list(K, name, obj);
	return obj;
    } else {
	TValue dummy = kcons(K, KINERT, KNIL);
	TValue last_pair = dummy;
	TValue tail = obj;
    
	while(ttispair(tail) && !kis_marked(tail)) {
	    TValue new_pair = kcons(K, kcar(tail), KNIL);
	    /* record the corresponding pair to simplify cycle handling */
	    kset_mark(tail, new_pair);
	    kset_cdr(last_pair, new_pair);
	    last_pair = new_pair;
	    tail = kcdr(tail);
	}

	if (ttispair(tail)) {
	    /* complete the cycle */
	    kset_cdr(last_pair, kget_mark(tail));
	}

	unmark_list(K, obj);

	if (!ttispair(tail) && !ttisnil(tail)) {
	    klispE_throw_extra(K, name , ": expected list"); 
	    return KINERT;
	} 
	return kcdr(dummy);
    }
}

/* check that obj is a list of environments and make a copy but don't keep 
   the cycles */
inline TValue check_copy_env_list(klisp_State *K, char *name, TValue obj)
{
    TValue dummy = kcons(K, KINERT, KNIL);
    TValue last_pair = dummy;
    TValue tail = obj;
    
    while(ttispair(tail) && !kis_marked(tail)) {
	TValue first = kcar(tail);
	if (!ttisenvironment(first)) {
	    klispE_throw_extra(K, name, ": not an environment in parent list");
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
	klispE_throw_extra(K, name , ": expected list"); 
	return KINERT;
    } 
    return kcdr(dummy);
}

/*
** Generic function for type predicates
** It can only be used by types that have a unique tag
*/
void typep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/*
** Generic function for type predicates
** It takes an arbitrary function pointer of type bool (*fn)(TValue o)
*/
void ftypep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/*
** Generic function for typed predicates (like char-alphabetic? or finite?)
** A typed predicate is a predicate that requires its arguments to be a certain
** type. This takes a function pointer for the type & one for the predicate,
** both of the same type: bool (*fn)(TValue o).
** On zero operands this return true
*/
void ftyped_predp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/*
** Generic function for typed binary predicates (like =? & char<?)
** A typed predicate is a predicate that requires its arguments to be a certain
** type. This takes a function pointer for the type bool (*typep)(TValue o) 
** & one for the predicate: bool (*fn)(TValue o1, TValue o2).
** This assumes the predicate is transitive and works even in cyclic lists
** On zero and one operand this return true
*/
void ftyped_bpredp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

#endif
