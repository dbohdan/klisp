/*
** kground.c
** Bindings in the ground environment
** See Copyright Notice in klisp.h
*/

/* TODO: split in different files for each module */

#include <assert.h>

#include "kstate.h"
#include "kobject.h"
#include "kground.h"
#include "kpair.h"
#include "kstring.h"
#include "kenvironment.h"
#include "kcontinuation.h"
#include "ksymbol.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kerror.h"

/*
** Some helper macros and functions
*/

#define anytype(obj_) (true)

/*
** NOTE: these are intended to be used at the beginning of a function
**   they expand to more than one statement and may evaluate some of
**   their arguments more than once 
*/
#define bind_1p(K_, n_, ptree_, v_) \
    bind_1tp(K_, n_, ptree_, "any", anytype, v_)

#define bind_1tp(K_, n_, ptree_, tstr_, t_, v_)	\
    TValue v_; \
    if (!ttispair(ptree_) || !ttisnil(kcdr(ptree_))) { \
	klispE_throw_extra(K_, n_ , ": Bad ptree (expected one argument)"); \
	return; \
    } \
    v_ = kcar(ptree_); \
    if (!t_(v_)) { \
	klispE_throw_extra(K_, n_ , ": Bad type on first argument " \
			   "(expected "	tstr_ ")");     \
	return; \
    } 


#define bind_2p(K_, n_, ptree_, v1_, v2_)		\
    bind_2tp(K_, n_, ptree_, "any", anytype, v1_, "any", anytype, v2_)

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

#define bind_3p(K_, n_, ptree_, v1_, v2_, v3_)		\
    TValue v1_, v2_, v3_;					\
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_)) || \
	  !ttispair(kcddr (ptree_)) || !ttisnil(kcdddr(ptree_))) {  \
	klispE_throw_extra(K_, n_, ": Bad ptree (expected three arguments)"); \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcadr(ptree_); \
    v3_ = kcaddr(ptree_)

/* bind at least 2 parameters, like (v1_ v2_ . v3_) */
#define bind_al2p(K_, n_, ptree_, v1_, v2_, v3_)		\
    TValue v1_, v2_, v3_;					\
    if (!ttispair(ptree_) || !ttispair(kcdr(ptree_))) {		       \
	klispE_throw_extra(K_, n_, ": Bad ptree (expected at least 2 " \
			   "arguments)");			       \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcadr(ptree_); \
    v3_ = kcddr(ptree_)

/* TODO: add name and source info */
#define make_operative(K_, fn_, ...) \
    kmake_operative(K_, KNIL, KNIL, fn_, __VA_ARGS__)
#define make_applicative(K_, fn_, ...) \
    kwrap(K_, kmake_operative(K_, KNIL, KNIL, fn_, __VA_ARGS__))

/*
** BEWARE: this is highly unhygienic, it assumes variables "symbol" and
** "value", both of type TValue. symbol will be bound to a symbol named by
** "n_" and can be referrenced in the var_args
*/
#define add_operative(K_, env_, n_, fn_, ...)	\
    { symbol = ksymbol_new(K_, n_); \
    value = make_operative(K_, fn_, __VA_ARGS__); \
    kadd_binding(K_, env_, symbol, value); }

#define add_applicative(K_, env_, n_, fn_, ...)	\
    { symbol = ksymbol_new(K_, n_); \
    value = make_applicative(K_, fn_, __VA_ARGS__); \
    kadd_binding(K_, env_, symbol, value); }

/*
** This states are useful for traversing trees, saving the state in the
** token char buffer
*/
#define ST_PUSH ((char) 0)
#define ST_CAR ((char) 1)
#define ST_CDR ((char) 2)

/*
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
** This is a generic function for type predicates
** It can only be used by types that have a unique tag
*/
void typep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: type tag (as by i2tv)
    */
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
	char *name = ksymbol_buf(xparams[0]);
	klispE_throw_extra(K, name, ": expected list");
	return;
    }
}

/*
** This section will roughly follow the report and will reference the
** section in which each symbol is defined
*/

/*
**
** 4 Core types and primitive features
**
*/

/*
** 4.1 Booleans
*/

/* 4.1.1 boolean? */
/* uses typep */

/*
** 4.2 Equivalence under mutation
*/

/* 4.2.1 eq? */

/* Helper (also used in equal?) */
inline bool eq2p(klisp_State *K, TValue obj1, TValue obj2);

/* TEMP: for now it takes only two argument */
void eqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_2p(K, "eq?", ptree, obj1, obj2);

    bool res = eq2p(K, obj1, obj2);
    kapply_cc(K, b2tv(res));
}

/* TEMP: for now this is the same as tv_equal,
   later it will change with numbers and immutable objects */
inline bool eq2p(klisp_State *K, TValue obj1, TValue obj2)
{
    return (tv_equal(obj1, obj2));
}

/*
** 4.3 Equivalence up to mutation
*/

/* 4.3.1 equal? */

/* TEMP: for now it takes only two argument */
/*
** equal? is O(n) where n is the number of pairs.
** Based on [1] "A linear algorithm for testing equivalence of finite automata"
** by J.E. Hopcroft and R.M.Karp
** List merging from [2] "A linear list merging algorithm"
** by J.E. Hopcroft and J.D. Ullman
** Idea to look up these papers from srfi 85: 
** "Recursive Equivalence Predicates" by William D. Clinger
*/

/* Helper */
/* compare two objects and check to see if they are "equal?". */
inline bool equal2p(klisp_State *K, TValue obj1, TValue obj2);

void equalp(klisp_State *K, TValue *xparas, TValue ptree, TValue denv)
{
    (void) denv;
    bind_2p(K, "equal?", ptree, obj1, obj2);
    bool res = equal2p(K, obj1, obj2);
    kapply_cc(K, b2tv(res));
}


/*
** Helpers
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
*/

/* find "name" of the set of this obj, if there isn't one create it,
   if there is one, flatten its branch */
inline TValue equal_find(klisp_State *K, TValue obj)
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
inline void equal_merge(klisp_State *K, TValue root1, TValue root2)
{
    /* K isn't needed but added for consistency */
    (void)K;
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
inline bool equal_find2_mergep(klisp_State *K, TValue obj1, TValue obj2)
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
inline bool equal2p(klisp_State *K, TValue obj1, TValue obj2)
{
    assert(ks_sisempty(K));

    /* the stack has the elements to be compaired, always in pairs.
       So the top should be compared with the one below, the third with
       the fourth and so on */
    ks_spush(K, obj1);
    ks_spush(K, obj2);

    /* if the stacks becomes empty, all pairs of elements were equal */
    bool result = true;

    while(!ks_sisempty(K)) {
	obj2 = ks_spop(K);
	obj1 = ks_spop(K);

	if (!eq2p(K, obj1, obj2)) {
	    if (ttispair(obj1) && ttispair(obj2)) {
		/* if they were already compaired, consider equal for now 
		   otherwise they are equal if both their cars and cdrs are */
		if (!equal_find2_mergep(K, obj1, obj2)) {
		    ks_spush(K, kcdr(obj1));
		    ks_spush(K, kcdr(obj2));
		    ks_spush(K, kcar(obj1));
		    ks_spush(K, kcar(obj2));
		}
	    } else if (ttisstring(obj1) && ttisstring(obj2)) {
		if (!kstring_equalp(obj1, obj2)) {
		    result = false;
		    break;
		}
	    } else {
		result = false;
		break;
	    }
	}
    }

    /* if result is false, the stack may not be empty */
    ks_sclear(K);

    unmark_tree(K, obj1);
    unmark_tree(K, obj2);
    
    return result;
}


/*
** 4.4 Symbols
*/

/* 4.4.1 symbol? */
/* uses typep */

/*
** 4.5 Control
*/

/* 4.5.1 inert? */
/* uses typep */

/* 4.5.2 $if */

/* helpers */
void select_clause(klisp_State *K, TValue *xparams, TValue obj);

/*  ASK JOHN: both clauses should probably be copied (copy-es-immutable) */
void Sif(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_3p(K, "$if", ptree, test, cons_c, alt_c);

    TValue new_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, select_clause, 
			   3, denv, cons_c, alt_c);

    klispS_set_cc(K, new_cont);
    ktail_eval(K, test, denv);
}

void select_clause(klisp_State *K, TValue *xparams, TValue obj)
{
    /*
    ** xparams[0]: dynamic env
    ** xparams[1]: consequent clause
    ** xparams[2]: alternative clause
    */
    if (ttisboolean(obj)) {
	TValue denv = xparams[0];
	TValue clause = bvalue(obj)? xparams[1] : xparams[2];
	ktail_eval(K, clause, denv);
    } else {
	klispE_throw(K, "$if: test is not a boolean");
	return;
    }
}

/*
** 4.6 Pairs and lists
*/

/* 4.6.1 pair? */
/* uses typep */

/* 4.6.2 null? */
/* uses typep */
    
/* 4.6.3 cons */
void cons(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2p(K, "cons", ptree, car, cdr);
    
    TValue new_pair = kcons(K, car, cdr);
    kapply_cc(K, new_pair);
}

/*
** 4.7 Pair mutation
*/

/* 4.7.1 set-car!, set-cdr! */
void set_carB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2tp(K, "set-car!", ptree, "pair", ttispair, pair, 
	     "any", anytype, new_car);

    if(!kis_mutable(pair)) {
	    klispE_throw(K, "set-car!: immutable pair");
	    return;
    }
    kset_car(pair, new_car);
    kapply_cc(K, KINERT);
}

void set_cdrB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2tp(K, "set-cdr!", ptree, "pair", ttispair, pair, 
	     "any", anytype, new_cdr);
    
    if(!kis_mutable(pair)) {
	    klispE_throw(K, "set-cdr!: immutable pair");
	    return;
    }
    kset_cdr(pair, new_cdr);
    kapply_cc(K, KINERT);
}

/* 4.7.2 copy-es-immutable */

/* Helper (also used by $vau, $lambda, etc) */
TValue copy_es_immutable_h(klisp_State *K, char *name, TValue ptree);

void copy_es_immutable(klisp_State *K, TValue *xparams, 
		       TValue ptree, TValue denv)
{
    /*
    ** xparams[0]: copy-es-immutable symbol
    */
    char *name = ksymbol_buf(xparams[0]);
    bind_1p(K, name, ptree, obj);

    TValue copy = copy_es_immutable_h(K, name, obj);
    kapply_cc(K, copy);
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
*/

TValue copy_es_immutable_h(klisp_State *K, char *name, TValue obj)
{
    /* 
    ** GC: obj is rooted because it is in the stack at all times.
    ** The copied pair should be kept safe some other way
    */
    TValue copy = obj;

    assert(ks_sisempty(K));
    assert(ks_tbisempty(K));

    ks_spush(K, obj);
    ks_tbpush(K, ST_PUSH);

    while(!ks_sisempty(K)) {
	char state = ks_tbpop(K);
	TValue top = ks_spop(K);

	if (state == ST_PUSH) {
	    if (ttispair(top) && kis_mutable(top)) {
		if (kis_marked(top)) {
		    /* this pair was already seen, use the same */
		    copy = kget_mark(top);
		} else {
		    TValue new_pair = kdummy_imm_cons(K);
		    kset_mark(top, new_pair);
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
		kset_car(new_pair, copy);
		/* leave the pair on the stack, continue with the cdr */
		ks_spush(K, top);
		ks_tbpush(K, ST_CDR);

		ks_spush(K, kcdr(top));
		ks_tbpush(K, ST_PUSH);
	    } else {
		kset_cdr(new_pair, copy);
		copy = new_pair;
	    }
	}
    }
    unmark_tree(K, obj);
    return copy;
}


/*
** 4.8 Environments
*/

/* 4.8.1 environment? */
/* uses typep */

/* 4.8.2 ignore? */
/* uses typep */

/* 4.8.3 eval */
void eval(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    bind_2tp(K, "eval", ptree, "any", anytype, expr,
	     "environment", ttisenvironment, env);

    ktail_eval(K, expr, env);
}

/* 4.8.4 make-environment */
/* TODO: let it accept any number of parameters */
void make_environment(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    TValue new_env;
    if (ttisnil(ptree)) {
	new_env = kmake_empty_environment(K);
	kapply_cc(K, new_env);
    } else if (ttispair(ptree) && ttisnil(kcdr(ptree))) {
	/* special common case of one parent, don't keep a list */
	TValue parent = kcar(ptree);
	if (ttisenvironment(parent)) {
	    new_env = kmake_environment(K, parent);
	    kapply_cc(K, new_env);
	} else {
	    klispE_throw(K, "make-environment: not an environment in "
			 "parent list");
	    return;
	}
    } else {
	/* this is the general case, copy the list but without the
	   cycle if there is any */
	TValue parents = check_copy_env_list(K, "make-environment", ptree);
	new_env = kmake_environment(K, parents);
	kapply_cc(K, new_env);
    }
}

/*
** 4.9 Environment mutation
*/

/* helpers */
inline void match(klisp_State *K, char *name, TValue env, TValue ptree, 
		  TValue obj);
void do_match(klisp_State *K, TValue *xparams, TValue obj);
inline void ptree_clear_all(klisp_State *K, TValue sym_ls);
inline TValue check_copy_ptree(klisp_State *K, char *name, TValue ptree, 
			       TValue penv);

/* 4.9.1 $define! */
void SdefineB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0] = define symbol
    */
    bind_2p(K, "$define!", ptree, dptree, expr);
    
    TValue def_sym = xparams[0];

    dptree = check_copy_ptree(K, "$define!", dptree, KIGNORE);
	
    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					 do_match, 3, dptree, denv, 
					 def_sym);
    kset_cc(K, new_cont);
    ktail_eval(K, expr, denv);
}

/* helpers */

/*
** This checks that the ptree parameter is a valid ptree and checks that the 
** environment parameter is either a symbol that is not also in ptree. or
** #ignore. It also copies the ptree so that it can't be mutated.
**
** A valid ptree must comply with the following: 
**   1) <ptree> -> <symbol> | #ignore | () | (<ptree> . <ptree>)
**   2) no symbol appears more than once in ptree
**   3) there is no cycle
**   NOTE: there may be diamonds, but no symbol should be reachable by more
**   than one path, see rule number 2
**
*/
inline TValue check_copy_ptree(klisp_State *K, char *name, TValue ptree, 
    TValue penv)
{
    /* 
    ** GC: ptree is rooted because it is in the stack at all times.
    ** The copied pair should be kept safe some other way
    ** the same for ptree
    */

    /* copy is only valid if the state isn't ST_PUSH */
    /* but init anyways to avoid warning */
    TValue copy = ptree;

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
		if (kis_marked(top)) {
		    /* TODO add symbol name */
		    ptree_clear_all(K, sym_ls);
		    klispE_throw_extra(K, name, ": repeated symbol in ptree");
		    /* avoid warning */
		    return KNIL;
		} else {
		    copy = top;
		    /* add it to the symbol list */
		    kset_mark(top, sym_ls);
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
			TValue new_pair = kdummy_imm_cons(K);
			kset_mark(top, new_pair);
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
		    klispE_throw_extra(K, name, ": cycle detected in ptree");
		    /* avoid warning */
		    return KNIL;
		}
		break;
	    }
	    default:
		ptree_clear_all(K, sym_ls);
		klispE_throw_extra(K, name, ": bad object type in ptree");
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
		    kset_car(copied_pair, copy);
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
		    kset_cdr(copied_pair, copy);
		}
		copy = copied_pair;
	    }
	}
    }

    if (ttissymbol(penv)) {
	if (kis_marked(penv)) {
	    /* TODO add symbol name */
	    ptree_clear_all(K, sym_ls);
	    klispE_throw_extra(K, name, ": same symbol in both ptree and "
			       "environment parmameter");
	}
    } else if (!ttisignore(penv)) {
	    /* TODO add symbol name */
	    ptree_clear_all(K, sym_ls);
	    klispE_throw_extra(K, name, ": symbol or #ignore expected as "
			       "environment parmameter");
    }
    ptree_clear_all(K, sym_ls);
    return copy;
}

/*
** Clear all the marks (symbols + pairs) & stacks.
** The stack should contain only pairs, sym_ls should be
** as above 
*/    
inline void ptree_clear_all(klisp_State *K, TValue sym_ls)
{
    while(!ttisnil(sym_ls)) {
	TValue first = sym_ls;
	sym_ls = kget_mark(first);
	kunmark(first);
    }

    while(!ks_sisempty(K)) {
	kunmark(ks_sget(K));
	ks_sdpop(K);
    }

    ks_tbclear(K);
}

inline void match(klisp_State *K, char *name, TValue env, TValue ptree, 
		  TValue obj)
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
		klispE_throw_extra(K, name, ": ptree doesn't match arguments");
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
		klispE_throw_extra(K, name, ": ptree doesn't match arguments");
		return;
	    }
	    break;
	default:
	    /* can't really happen */
	    break;
	}
    }
}

void do_match(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: ptree
    ** xparams[1]: dynamic environment
    ** xparams[2]: combiner symbol
    */
    TValue ptree = xparams[0];
    TValue env = xparams[1];
    char *name = ksymbol_buf(xparams[2]);

    match(K, name, env, ptree, obj);
    kapply_cc(K, KINERT);
}

/*
** 4.10 Combiners
*/

/* 4.10.1 operative? */
/* uses typep */

/* 4.10.2 applicative? */
/* uses typep */

/* 4.10.3 $vau */
/* 5.3.1 $vau */

/* Helper (also used by $sequence and $lambda) */
void do_seq(klisp_State *K, TValue *xparams, TValue obj);
void do_vau(klisp_State *K, TValue *xparams, TValue obj, TValue denv);

void Svau(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    bind_al2p(K, "$vau", ptree, vptree, vpenv, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, "$vau", vptree, vpenv);
    /* the body should be a list */
    (void)check_list(K, "$vau", vbody);
    vbody = copy_es_immutable_h(K, "$vau", vbody);

    TValue new_op = make_operative(K, do_vau, 4, vptree, vpenv, vbody, denv);
    kapply_cc(K, new_op);
}

/* the ramaining list can't be null, that case is managed before */
void do_seq(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: remaining list
    ** xparams[1]: dynamic environment
    */
    TValue ls = xparams[0];
    TValue first = kcar(ls);
    TValue tail = kcdr(ls);
    TValue denv = xparams[1];

    if (ttispair(tail)) {
	TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, denv);
	kset_cc(K, new_cont);
    }
    ktail_eval(K, first, denv);
}

void do_vau(klisp_State *K, TValue *xparams, TValue obj, TValue denv)
{
    /*
    ** xparams[0]: ptree
    ** xparams[1]: penv
    ** xparams[2]: body
    ** xparams[3]: senv
    */
    TValue ptree = xparams[0];
    TValue penv = xparams[1];
    TValue body = xparams[2];
    TValue senv = xparams[3];

    /* bindings in an operative are in a child of the static env */
    TValue env = kmake_environment(K, senv);
    /* TODO use name from operative */
    match(K, "[user-operative]", env, ptree, obj);
    kadd_binding(K, env, penv, denv);
    
    if (ttisnil(body)) {
	kapply_cc(K, KINERT);
    } else {
	/* this is needed because seq continuation doesn't check for 
	   nil sequence */
	TValue tail = kcdr(body);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, env);
	    kset_cc(K, new_cont);
	} 
	ktail_eval(K, kcar(body), env);
    }
}

/* 4.10.4 wrap */
void wrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1tp(K, "wrap", ptree, "combiner", ttiscombiner, comb);
    TValue new_app = kwrap(K, comb);
    kapply_cc(K, new_app);
}

/* 4.10.5 unwrap */
void unwrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1tp(K, "unwrap", ptree, "applicative", ttisapplicative, app);
    TValue underlying = kunwrap(K, app);
    kapply_cc(K, underlying);
}

/*
**
** 5 Core library features (I)
**
*/

/*
** 5.1 Control
*/

/* 5.1.1 $sequence */
void Ssequence(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;

    if (ttisnil(ptree)) {
	kapply_cc(K, KINERT);
    } else {
	/* the list of instructions is copied to avoid mutation */
	/* MAYBE: copy the evaluation structure, ASK John */
	TValue ls = check_copy_list(K, "$sequence", ptree);
	/* this is needed because seq continuation doesn't check for 
	   nil sequence */
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, denv);
	    kset_cc(K, new_cont);
	} 
	ktail_eval(K, kcar(ls), denv);
    }
}

/*
** 5.2 Pairs and lists
*/

/* 5.2.1 list */
/* TODO */

/* 5.2.2 list* */
/* TODO */

/*
** 5.3 Combiners
*/

/* 5.3.1 $vau */
/* DONE: above, together with 4.10.4 */

/* 5.3.2 $lambda */
void Slambda(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    bind_al2p(K, "$lambda", ptree, vptree, vpenv, vbody);

    /* The ptree & body are copied to avoid mutation */
    vptree = check_copy_ptree(K, "$lambda", vptree, vpenv);
    /* the body should be a list */
    (void)check_list(K, "$lambda", vbody);
    vbody = copy_es_immutable_h(K, "$lambda", vbody);

    TValue new_app = make_applicative(K, do_vau, 4, vptree, vpenv, vbody, denv);
    kapply_cc(K, new_app);
}



/*
** This is called once to bind all symbols in the ground environment
*/
TValue kmake_ground_env(klisp_State *K)
{
    TValue ground_env = kmake_empty_environment(K);

    TValue symbol, value;

   /*
   ** TODO: this pattern could be abstracted away with a 
   ** non-hygienic macro (that inserted names "symbol" and "value")
   */

    /*
    ** This section will roughly follow the report and will reference the
    ** section in which each symbol is defined
    */

    /*
    **
    ** 4 Core types and primitive features
    **
    */

    /*
    ** 4.1 Booleans
    */

    /* 4.1.1 boolean? */
    add_applicative(K, ground_env, "boolean?", typep, 2, symbol, 
		    i2tv(K_TBOOLEAN));

    /*
    ** 4.2 Equivalence under mutation
    */

    /* 4.2.1 eq? */
    add_applicative(K, ground_env, "eq?", eqp, 0);

    /*
    ** 4.3 Equivalence up to mutation
    */

    /* 4.3.1 equal? */
    add_applicative(K, ground_env, "equal?", equalp, 0);

    /*
    ** 4.4 Symbols
    */

    /* 4.4.1 symbol? */
    add_applicative(K, ground_env, "symbol?", typep, 2, symbol, 
		    i2tv(K_TSYMBOL));

    /*
    ** 4.5 Control
    */

    /* 4.5.1 inert? */
    add_applicative(K, ground_env, "inert?", typep, 2, symbol, 
		    i2tv(K_TINERT));

    /* 4.5.2 $if */
    add_operative(K, ground_env, "$if", Sif, 0);

    /*
    ** 4.6 Pairs and lists
    */

    /* 4.6.1 pair? */
    add_applicative(K, ground_env, "pair?", typep, 2, symbol, 
		    i2tv(K_TPAIR));

    /* 4.6.2 null? */
    add_applicative(K, ground_env, "null?", typep, 2, symbol, 
		    i2tv(K_TNIL));
    
    /* 4.6.3 cons */
    add_applicative(K, ground_env, "cons", cons, 0);

    /*
    ** 4.7 Pair mutation
    */

    /* 4.7.1 set-car!, set-cdr! */
    add_applicative(K, ground_env, "set-car!", set_carB, 0);
    add_applicative(K, ground_env, "set-cdr!", set_cdrB, 0);

    /* 4.7.2 copy-es-immutable */
    add_applicative(K, ground_env, "copy-es-immutable", copy_es_immutable, 
		    1, symbol);

    /*
    ** 4.8 Environments
    */

    /* 4.8.1 environment? */
    add_applicative(K, ground_env, "environment?", typep, 2, symbol, 
		    i2tv(K_TENVIRONMENT));

    /* 4.8.2 ignore? */
    add_applicative(K, ground_env, "ignore?", typep, 2, symbol, 
		    i2tv(K_TIGNORE));

    /* 4.8.3 eval */
    add_applicative(K, ground_env, "eval", eval, 0);

    /* 4.8.4 make-environment */
    add_applicative(K, ground_env, "make-environment", make_environment, 0);

    /*
    ** 4.9 Environment mutation
    */

    /* 4.9.1 $define! */
    add_operative(K, ground_env, "$define!", SdefineB, 1, symbol);

    /*
    ** 4.10 Combiners
    */

    /* 4.10.1 operative? */
    add_applicative(K, ground_env, "operative?", typep, 2, symbol, 
		    i2tv(K_TOPERATIVE));

    /* 4.10.2 applicative? */
    add_applicative(K, ground_env, "applicative?", typep, 2, symbol, 
		    i2tv(K_TAPPLICATIVE));

    /* 4.10.3 $vau */
    add_operative(K, ground_env, "$vau", Svau, 0);

    /* 4.10.4 wrap */
    add_applicative(K, ground_env, "wrap", wrap, 0);

    /* 4.10.5 unwrap */
    add_applicative(K, ground_env, "unwrap", unwrap, 0);

    /*
    **
    ** 5 Core library features (I)
    **
    */

    /*
    ** 5.1 Control
    */

    /* 5.1.1 $sequence */
    add_operative(K, ground_env, "$sequence", Ssequence, 0);

    /*
    ** 5.2 Pairs and lists
    */

    /* 5.2.1 list */
    /* TODO */

    /* 5.2.2 list* */
    /* TODO */

    /*
    ** 5.3 Combiners
    */

    /* 5.3.1 $vau */
    /* DONE: above, together with 4.10.4 */

    /* 5.3.2 $lambda */
    add_operative(K, ground_env, "$lambda", Slambda, 0);

    return ground_env;
}
