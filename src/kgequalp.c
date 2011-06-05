/*
** kgequalp.h
** Equivalence up to mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kstring.h" /* for kstring_equalp */
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgeqp.h" /* for eq2p */
#include "kgequalp.h"

/* 4.3.1 equal? */
/* 6.6.1 equal? */

/*
** equal? is O(n) where n is the number of pairs.
** Based on [1] "A linear algorithm for testing equivalence of finite automata"
** by J.E. Hopcroft and R.M.Karp
** List merging from [2] "A linear list merging algorithm"
** by J.E. Hopcroft and J.D. Ullman
** Idea to look up these papers from srfi 85: 
** "Recursive Equivalence Predicates" by William D. Clinger
*/
void equalp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    UNUSED(xparams);

    int32_t pairs = check_list(K, "equal?", true, ptree, NULL);

    /* In this case we can get away without comparing the
       first and last element on a cycle because equal? is
       symetric, (cf: ftyped_bpred) */
    int32_t comps = pairs - 1;
    TValue tail = ptree;
    TValue res = KTRUE;
    while(comps-- > 0) {  /* comps could be -1 if ptree is nil */
	TValue first = kcar(tail);
	tail = kcdr(tail); /* tail only advances one place per iteration */
	TValue second = kcar(tail);

	if (!equal2p(K, first, second)) {
	    res = KFALSE;
	    break;
	}
    }

    kapply_cc(K, res);
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
**
** GC: all of these assume that arguments are rooted.
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
/* REFACTOR these ifs: compare both types first, then switch on type */
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

    unmark_tree(K, saved_obj1);
    unmark_tree(K, saved_obj2);
    
    return result;
}


/* init ground */
void kinit_equalp_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;
    /* 4.3.1 equal? */
    /* 6.6.1 equal? */
    add_applicative(K, ground_env, "equal?", equalp, 0);
}
