/*
** kground.c
** Bindings in the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

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

#include "kghelpers.h"
#include "kgbooleans.h"
#include "kgeqp.h"
#include "kgequalp.h"
#include "kgsymbols.h"
#include "kgcontrol.h"

/*
** This section will roughly follow the report and will reference the
** section in which each symbol is defined
*/
/* TODO: split in different files for each module */

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

/* Helper (also used by $lambda) */
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
** 5.2 Pairs and lists
*/

/* 5.2.1 list */
/* the underlying combiner of list return the complete ptree, the only list
   checking is implicit in the applicative evaluation */
void list(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    (void) denv;
    kapply_cc(K, ptree);
}

/* 5.2.2 list* */
/* TODO: 
   OPTIMIZE: if this call is a result of a call to eval, we could get away
   with just setting the kcdr of the next to last pair to the car of
   the last pair, because the list of operands is fresh. Also the type
   check wouldn't be necessary. This optimization technique could be
   used in lots of places to avoid checks and the like. */
void listS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) xparams;
    (void) denv;

    if (ttisnil(ptree)) {
	klispE_throw(K, "list*: empty argument list"); 
	return;
    }
    /* GC: should root dummy */
    TValue dummy = kcons(K, KINERT, KNIL);
    TValue last_pair = dummy;
    TValue tail = ptree;
    
    /* First copy the list, but remembering the next to last pair */
    while(ttispair(tail) && !kis_marked(tail)) {
	kmark(tail);
	/* we save the next_to last pair in the cdr to 
	   allow the change into an improper list later */
	TValue new_pair = kcons(K, kcar(tail), last_pair);
	kset_cdr(last_pair, new_pair);
	last_pair = new_pair;
	tail = kcdr(tail);
    }
    unmark_list(K, ptree);

    if (ttisnil(tail)) {
	/* Now eliminate the last pair to get the correct improper list.
	   This avoids an if in the above loop. It's inside the if because
	   we need at least one pair for this to work. */
	TValue next_to_last_pair = kcdr(last_pair);
	kset_cdr(next_to_last_pair, kcar(last_pair));
	kapply_cc(K, kcdr(dummy));
    } else if (ttispair(tail)) { /* cyclic argument list */
	klispE_throw(K, "list*: cyclic argument list"); 
	return;
    } else {
	klispE_throw(K, "list*: argument list is improper"); 
	return;
    }
}

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
** 5.4 Pairs and lists
*/

/* 5.4.1 car, cdr */
/* 5.4.2 caar, cadr, ... cddddr */

/* Helper macros to construct xparams[1] */
#define C_AD_R_PARAM(len_, br_) \
    (i2tv((C_AD_R_LEN(len_) | (C_AD_R_BRANCH(br_)))))
#define C_AD_R_LEN(len_) ((len_) << 4)
#define C_AD_R_BRANCH(br_) \
    ((br_ & 0x0001? 0x1 : 0) | \
     (br_ & 0x0010? 0x2 : 0) | \
     (br_ & 0x0100? 0x4 : 0) | \
     (br_ & 0x1000? 0x8 : 0))

/* the name stands for the regular expression c[ad]{1,4}r */
void c_ad_r( klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{

    /*
    ** xparams[0]: name as symbol
    ** xparams[1]: an int with the less significant 2 nibbles 
    **             standing for the count and the branch selection.
    **             The high nibble is the count: that is the number of
    **             'a's and 'd's in the name, for example:
    **             0x1? for car and cdr.
    **             0x2? for caar, cadr, cdar and cddr.
    **             The low nibble is the branch selection, a 0 bit means
    **             car, a 1 bit means cdr, the first bit to be applied 
    **             is bit 0 so: caar=0x20, cadr=0x21, cdar:0x22, cddr 0x23
    */

    char *name = ksymbol_buf(xparams[0]);
    int p = ivalue(xparams[1]);
    int count = (p >> 4) & 0xf;
    int branches = p & 0xf;

    bind_1p(K, name, ptree, obj);

    while(count) {
	if (!ttispair(obj)) {
	    klispE_throw_extra(K, name, ": non pair found while traversing"); 
	    return;
	}
	obj = ((branches & 1) == 0)? kcar(obj) : kcdr(obj);
	branches >>= 1;
	--count;
    }
    kapply_cc(K, obj);
}


/*
** 5.5 Combiners
*/

/* 5.5.1 apply */
void apply(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_al2p(K, "apply", ptree, app, obj, maybe_env);

    if(!ttisapplicative(app)) {
	klispE_throw(K, "apply: Bad type on first argument "
		     "(expected applicative)");    
	return;
    }
    TValue env;
    /* TODO move to an inlinable function */
    if (ttisnil(maybe_env)) {
	env = kmake_empty_environment(K);
    } else if (ttispair(maybe_env) && ttisnil(kcdr(maybe_env))) {
	env = kcar(maybe_env);
	if (!ttisenvironment(env)) {
	    klispE_throw(K, "apply: Bad type on optional argument "
			 "(expected environment)");    
	}
    } else {
	klispE_throw(K, "apply: Bad ptree structure (in optional argument)");
    }
    TValue expr = kcons(K, kunwrap(K, app), obj);
    ktail_eval(K, expr, env);
}

/*
** 5.7 Pairs and lists
*/

/* 5.7.1 get-list-metrics */
void get_list_metrics(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_1p(K, "get-list-metrics", ptree, obj);
    int32_t pairs = 0;
    TValue tail = obj;

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

    /* GC: root intermediate pairs */
    TValue res = kcons(K, i2tv(apairs), kcons(K, i2tv(cpairs), KNIL));
    res = kcons(K, i2tv(pairs), kcons(K, i2tv(nils), res));
    kapply_cc(K, res);
}

/* 5.7.2 list-tail */
/* ASK John: can the object be a cyclic list? the wording of the report
   seems to indicate that can't be the case, but it makes sense here 
   (cf $encycle!) to allow cyclic lists, so that's what I do */
void list_tail(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    /* XXX: should be integer instead of fixint, but that's all
       we have for now */
    bind_2tp(K, "list-tail", ptree, "any", anytype, obj,
	     "finite integer", ttisfixint, tk);
    int k = ivalue(tk);
    if (k < 0) {
	klispE_throw(K, "list-tail: negative index");
	return;
    }

    while(k) {
	if (!ttispair(obj)) {
	    klispE_throw(K, "list-tail: non pair found while traversing "
			 "object");
	    return;
	}
	obj = kcdr(obj);
	--k;
    }
    kapply_cc(K, obj);
}

/*
** 5.8 Pair mutation
*/

/* 5.8.1 encycle! */
/* ASK John: can the object be a cyclic list of length less than k1+k2? 
   the wording of the report seems to indicate that can't be the case, 
   and here it makes sense to forbid it because otherwise the list-metrics 
   of the result would differ with the expected ones (cf list-tail). 
   So here an error is signaled if the improper list cyclic with less pairs
   than needed */
void encycleB(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    (void) xparams;
    /* XXX: should be integer instead of fixint, but that's all
       we have for now */
    bind_3tp(K, "encycle!", ptree, "any", anytype, obj,
	     "finite integer", ttisfixint, tk1,
	     "finite integer", ttisfixint, tk2);

    int32_t k1 = ivalue(tk1);
    int32_t k2 = ivalue(tk2);

    if (k1 < 0 || k2 < 0) {
	klispE_throw(K, "encycle!: negative index");
	return;
    }

    TValue tail = obj;

    while(k1) {
	if (!ttispair(tail)) {
	    unmark_list(K, obj);
	    klispE_throw(K, "encycle!: non pair found while traversing "
			 "object");
	    return;
	} else if (kis_marked(tail)) {
	    unmark_list(K, obj);
	    klispE_throw(K, "encycle!: too few pairs in cyclic list");
	    return;
	}
	kmark(tail);
	tail = kcdr(tail);
	--k1;
    }

    TValue fcp = tail;

    /* if k2 == 0 do nothing (but this still checks that the obj
       has at least k1 pairs */
    if (k2 != 0) {
	--k2; /* to have cycle length k2 we should discard k2-1 pairs */
	while(k2) {
	    if (!ttispair(tail)) {
		unmark_list(K, obj);
		klispE_throw(K, "encycle!: non pair found while traversing "
			     "object");
		return;
	    } else if (kis_marked(tail)) {
		unmark_list(K, obj);
		klispE_throw(K, "encycle!: too few pairs in cyclic list");
		return;
	    }
	    kmark(tail);
	    tail = kcdr(tail);
	    --k2;
	}
	if (!kis_mutable(tail)) {
	    unmark_list(K, obj);
	    klispE_throw(K, "encycle!: immutable pair");
	    return;
	} else {
	    kset_cdr(tail, fcp);
	}
    }
    unmark_list(K, obj);
    kapply_cc(K, KINERT);
}

/*
** 5.9 Combiners
*/

/* 5.9.1 map */
/* TODO */

/*
** 5.10 Environments
*/

/* 5.10.1 $let */
/* TODO */

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
    /* 5.3.1 $vau */
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
    add_applicative(K, ground_env, "list", list, 0);

    /* 5.2.2 list* */
    add_applicative(K, ground_env, "list*", listS, 0);

    /*
    ** 5.3 Combiners
    */

    /* 5.3.1 $vau */
    /* DONE: above, together with 4.10.4 */

    /* 5.3.2 $lambda */
    add_operative(K, ground_env, "$lambda", Slambda, 0);

    /*
    ** 5.4 Pairs and lists
    */

    /* 5.4.1 car, cdr */
    add_applicative(K, ground_env, "car", c_ad_r, 2, symbol, 
		    C_AD_R_PARAM(1, 0x0000));
    add_applicative(K, ground_env, "cdr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(1, 0x0001));

    /* 5.4.2 caar, cadr, ... cddddr */
    add_applicative(K, ground_env, "caar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0000));
    add_applicative(K, ground_env, "cadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0001));
    add_applicative(K, ground_env, "cdar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0010));
    add_applicative(K, ground_env, "cddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0011));

    add_applicative(K, ground_env, "caaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0000));
    add_applicative(K, ground_env, "caadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0001));
    add_applicative(K, ground_env, "cadar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0010));
    add_applicative(K, ground_env, "caddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0011));
    add_applicative(K, ground_env, "cdaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0100));
    add_applicative(K, ground_env, "cdadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0101));
    add_applicative(K, ground_env, "cddar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0110));
    add_applicative(K, ground_env, "cdddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0111));

    add_applicative(K, ground_env, "caaaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0000));
    add_applicative(K, ground_env, "caaadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0001));
    add_applicative(K, ground_env, "caadar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0010));
    add_applicative(K, ground_env, "caaddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0011));
    add_applicative(K, ground_env, "cadaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0100));
    add_applicative(K, ground_env, "cadadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0101));
    add_applicative(K, ground_env, "caddar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0110));
    add_applicative(K, ground_env, "cadddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0111));
    add_applicative(K, ground_env, "cdaaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1000));
    add_applicative(K, ground_env, "cdaadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1001));
    add_applicative(K, ground_env, "cdadar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1010));
    add_applicative(K, ground_env, "cdaddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1011));
    add_applicative(K, ground_env, "cddaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1100));
    add_applicative(K, ground_env, "cddadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1101));
    add_applicative(K, ground_env, "cdddar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1110));
    add_applicative(K, ground_env, "cddddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1111));

    /*
    ** 5.5 Combiners
    */

    /* 5.5.1 apply */
    add_applicative(K, ground_env, "apply", apply, 0);

    /*
    ** 5.6 Control
    */

    /* 5.6.1 $cond */
    /* TODO */

    /*
    ** 5.7 Pairs and lists
    */

    /* 5.7.1 get-list-metrics */
    add_applicative(K, ground_env, "get-list-metrics", get_list_metrics, 0);

    /* 5.7.2 list-tail */
    add_applicative(K, ground_env, "list-tail", list_tail, 0);

    /*
    ** 5.8 Pair mutation
    */

    /* 5.8.1 encycle! */
    add_applicative(K, ground_env, "encycle!", encycleB, 0);

    /*
    ** 5.9 Combiners
    */

    /* 5.9.1 map */
    /* TODO */

    /*
    ** 5.10 Environments
    */

    /* 5.10.1 $let */
    /* TODO */

    return ground_env;
}
