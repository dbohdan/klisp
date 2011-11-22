/*
** kgenv_mut.h
** Environment mutation features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgenv_mut_h
#define kgenv_mut_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* helpers */
inline void match(klisp_State *K, char *name, TValue env, TValue ptree, 
		  TValue obj);
void do_match(klisp_State *K);
inline void ptree_clear_all(klisp_State *K, TValue sym_ls);
inline TValue check_copy_ptree(klisp_State *K, char *name, TValue ptree, 
			       TValue penv);
/* 4.9.1 $define! */
void SdefineB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* MAYBE: don't make these inline */
/*
** Clear all the marks (symbols + pairs) & stacks.
** The stack should contain only pairs, sym_ls should be
** as above 
*/    
inline void ptree_clear_all(klisp_State *K, TValue sym_ls)
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
inline TValue check_copy_ptree(klisp_State *K, char *name, TValue ptree, 
			       TValue penv)
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

/* 6.8.1 $set! */
void SsetB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper for $set! */
void do_set_eval_obj(klisp_State *K);

/* Helpers for $provide & $import! */
TValue check_copy_symbol_list(klisp_State *K, char *name, TValue obj);
void do_import(klisp_State *K);

/* 6.8.2 $provide! */
void SprovideB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.8.3 $import! */
void SimportB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* init ground */
void kinit_env_mut_ground_env(klisp_State *K);

#endif
