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
	klispE_throw_extra(K_, n_, ": Bad ptree (expected tree arguments)"); \
	return; \
    } \
    v1_ = kcar(ptree_); \
    v2_ = kcadr(ptree_); \
    v3_ = kcaddr(ptree_)

/* TODO: add name and source info */
#define kmake_applicative(K_, fn_, ...) \
    kwrap(K_, kmake_operative(K_, KNIL, KNIL, fn_, __VA_ARGS__))


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
/* TEMP: for now it takes a single argument */
void booleanp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "boolean?", ptree, o);
    kapply_cc(K, b2tv(ttisboolean(o)));
}

/*
** 4.2 Equivalence under mutation
*/

/* 4.2.1 eq? */
/* TEMP: for now it takes only two argument */
void eqp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2p(K, "eq?", ptree, o1, o2);
    /* TEMP: for now this is the same as 
       later it will change with numbers and immutable objects */
    kapply_cc(K, b2tv(tv_equal(o1, o2)));
}

/*
** 4.3 Equivalence up to mutation
*/

/* 4.3.1 equal? */
/* TEMP: for now it takes only two argument */
/* TODO */

/*
** 4.4 Symbols
*/

/* 4.4.1 symbol? */
void symbolp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "symbol?", ptree, o);
    kapply_cc(K, b2tv(ttissymbol(o)));
}

/*
** 4.5 Control
*/

/* 4.5.1 inert? */
void inertp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "inert?", ptree, o);
    kapply_cc(K, b2tv(ttisinert(o)));
}

/* 4.5.2 $if */

/* helpers */
void select_clause(klisp_State *K, TValue *xparams, TValue obj);

/* TODO: both clauses should probably be copied */
void Sif(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;

    bind_3p(K, "$if", ptree, test, cons_c, alt_c);

    TValue new_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, select_clause, 
			   3, denv, cons_c, alt_c);

    klispS_set_cc(K, new_cont);
    ktail_call(K, K->eval_op, test, denv);
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
	ktail_call(K, K->eval_op, clause, denv);
    } else {
	klispE_throw(K, "$if: test is not a boolean");
	return;
    }
}

/*
** 4.6 Pairs and lists
*/

/* 4.6.1 pair? */
void pairp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "pair?", ptree, o);
    kapply_cc(K, b2tv(ttispair(o)));
}

/* 4.6.2 null? */
void nullp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "null?", ptree, o);
    kapply_cc(K, b2tv(ttisnil(o)));
}
    
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
/* TODO: check if pair is immutable */
void set_carB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2tp(K, "set-car!", ptree, "pair", ttispair, pair, 
	     "any", anytype, new_car);
    
    kset_car(pair, new_car);
    kapply_cc(K, KINERT);
}

void set_cdrB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_2tp(K, "set-cdr!", ptree, "pair", ttispair, pair, 
	     "any", anytype, new_cdr);
    
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

#define CEI_ST_PUSH ((char) 0)
#define CEI_ST_CAR ((char) 1)
#define CEI_ST_CDR ((char) 2)

TValue copy_es_immutable_h(klisp_State *K, char *name, TValue obj)
{
    /* 
    ** GC: obj is rooted because it is in the stack at all times.
    ** The copied pair should be kept safe some other way
    */
    TValue copy = obj;

    ks_spush(K, obj);
    ks_tbpush(K, CEI_ST_PUSH);

    while(!ks_sisempty(K)) {
	char state = ks_tbpop(K);
    }

    return copy;
}


/*
** 4.8 Environments
*/

/* 4.8.1 environment? */
void environmentp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "environment?", ptree, o);
    kapply_cc(K, b2tv(ttisenvironment(o)));
}

/* 4.8.2 ignore? */
void ignorep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "ignore?", ptree, o);
    kapply_cc(K, b2tv(ttisignore(o)));
}

/* 4.8.3 eval */
void eval(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv)
{
    (void) denv;
    bind_2tp(K, "eval", ptree, "any", anytype, expr,
	     "environment", ttisenvironment, env);

    ktail_call(K, K->eval_op, expr, env);
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
	TValue parent = kcar(ptree);
	if (ttisenvironment(parent)) {
	    new_env = kmake_environment(K, parent);
	    kapply_cc(K, new_env);
	} else {
	    klispE_throw(K, "make-environment: Bad type on first "
			 "argument (expected environment)");
	    return;
	}
    } else {
	klispE_throw(K, "make-environment: Bad ptree (expected "
		     "zero or one argument");
	return;
    }
}

/*
** 4.9 Environment mutation
*/

/* helpers */
void match(klisp_State *K, TValue *xparams, TValue obj);
inline void ptree_clear_marks(klisp_State *K, TValue sym_ls);
inline TValue check_copy_ptree(klisp_State *K, char *name, TValue ptree);

/* 4.9.1 $define! */
void SdefineB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    /*
    ** xparams[0] = define symbol
    */
    bind_2p(K, "$define!", ptree, dptree, expr);
    
    TValue def_sym = xparams[0];

    dptree = check_copy_ptree(K, "$define!", dptree);
	
    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					 match, 3, dptree, denv, 
					 def_sym);
    kset_cc(K, new_cont);
    ktail_call(K, K->eval_op, expr, denv);
}

/* helpers */

/*
** This checks that ptree is a valid <ptree>: 
**   1) <ptree> -> <symbol> | #ignore | () | (<ptree> . <ptree>)
**   2) no symbol appears more than once in ptree
**   3) there is no cycle
**   NOTE: there may be diamonds, but no symbol should be reachable by more
**   than one path, see rule number 2
**
** It also copies the ptree so that it can't be mutated
** TODO: if ptree is immutable don't copy it
*/
inline TValue check_copy_ptree(klisp_State *K, char *name, TValue ptree)
{
    /* 
    ** GC: ptree is rooted because it is in the stack at all times.
    ** The copied pair should be kept safe some other way
    */
    TValue copy = ptree;

    /* 
    ** NIL terminated singly linked list of symbols 
    ** (using the mark as next pointer) 
    */
    TValue sym_ls = KNIL;

    assert(ks_sisempty(K));

    ks_spush(K, ptree);
    /* last operation was a push */
    bool push = true;

    while(!ks_sisempty(K)) {
	TValue top = ks_sget(K);

	if (push) {
            /* last operation was a push */
	    switch(ttype(top)) {
	    case K_TIGNORE:
	    case K_TNIL:
		ks_sdpop(K);
		push = false;
		copy = top;
		break;
	    case K_TSYMBOL: {
		if (kis_marked(top)) {
		    /* TODO add symbol name */
		    ks_sdpop(K);
		    ptree_clear_marks(K, sym_ls);
		    klispE_throw_extra(K, name, ": repeated symbol in ptree");
		    /* avoid warning */
		    return KNIL;
		} else {
		    ks_sdpop(K);
		    push = false;
		    copy = top;
		    /* add it to the symbol list */
		    kset_mark(top, sym_ls);
		    sym_ls = top;

		}
		break;
	    }
	    case K_TPAIR: {
		if (kis_unmarked(top)) {
		    kset_mark(top, i2tv(1));
		    /* create a new pair as copy, leave it above */
		    ks_spush(K, kdummy_cons(K));
		    ks_spush(K, kcar(top)); 
		    push = true;
		} else {
		    /* marked pair means a cycle was found */
		    ptree_clear_marks(K, sym_ls);
		    klispE_throw_extra(K, name, ": cycle detected in ptree");
		    /* avoid warning */
		    return KNIL;
		}
		break;
	    }
	    default:
		ks_sdpop(K);
		ptree_clear_marks(K, sym_ls);
		klispE_throw_extra(K, name, ": bad object type in ptree");
		/* avoid warning */
		return KNIL;
	    }
	} else { 
            /* last operation was a pop */
	    /* top is a copied obj, below that there must be a pair 
	       with either 1 or 2 as mark */
	    ks_sdpop(K);
	    TValue below = ks_sget(K);
	    int32_t mark = ivalue(kget_mark(below));
	    if (mark == 1) { 
		/* only car was checked (not yet copied) */
		kset_car(top, copy);
		kset_mark(below, i2tv(2));
		/* put the copied pair again */
		ks_spush(K, top); 
		ks_spush(K, kcdr(below)); 
		push = true;
	    } else { 
                /* both car & cdr were checked (cdr not yet copied) */
		/* the unmark is needed to allow diamonds */
		kunmark(below);
		kset_cdr(top, copy);
		copy = top;
		ks_sdpop(K);
		push = false;
	    }
	}
    }
    
    ptree_clear_marks(K, sym_ls);
    return copy;
}

/* The stack should contain only pairs, sym_ls should be
 as above */    
inline void ptree_clear_marks(klisp_State *K, TValue sym_ls)
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
}

void match(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: ptree
    ** xparams[1]: dynamic environment
    ** xparams[2]: combiner symbol
    */
    TValue ptree = xparams[0];
    TValue env = xparams[1];
    char *name = ksymbol_buf(xparams[2]);

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
    kapply_cc(K, KINERT);
}

/*
** 4.10 Combiners
*/

/* 4.10.1 operative? */
void operativep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "operative?", ptree, o);
    kapply_cc(K, b2tv(ttisoperative(o)));
}

/* 4.10.2 applicative? */
void applicativep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    (void) xparams;
    bind_1p(K, "applicative?", ptree, o);
    kapply_cc(K, b2tv(ttisapplicative(o)));
}

/* 4.10.3 $vau */
/* TODO */

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
    symbol = ksymbol_new(K, "boolean?");
    value = kmake_applicative(K, booleanp, 0);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.2 Equivalence under mutation
    */

    /* 4.2.1 eq? */
    symbol = ksymbol_new(K, "eq?");
    value = kmake_applicative(K, eqp, 0);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.3 Equivalence up to mutation
    */

    /* 4.3.1 equal? */
    /* TODO */

    /*
    ** 4.4 Symbols
    */

    /* 4.4.1 symbol? */
    symbol = ksymbol_new(K, "symbol?");
    value = kmake_applicative(K, symbolp, 0);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.5 Control
    */

    /* 4.5.1 inert? */
    symbol = ksymbol_new(K, "inert?");
    value = kmake_applicative(K, inertp, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.5.2 $if */
    symbol = ksymbol_new(K, "$if");
    value = kmake_operative(K, KNIL, KNIL, Sif, 0);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.6 Pairs and lists
    */

    /* 4.6.1 pair? */
    symbol = ksymbol_new(K, "pair?");
    value = kmake_applicative(K, pairp, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.6.2 null? */
    symbol = ksymbol_new(K, "null?");
    value = kmake_applicative(K, nullp, 0);
    kadd_binding(K, ground_env, symbol, value);
    
    /* 4.6.3 cons */
    symbol = ksymbol_new(K, "cons");
    value = kmake_applicative(K, cons, 0);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.7 Pair mutation
    */

    /* 4.7.1 set-car!, set-cdr! */
    symbol = ksymbol_new(K, "set-car!");
    value = kmake_applicative(K, set_carB, 0);
    kadd_binding(K, ground_env, symbol, value);
    symbol = ksymbol_new(K, "set-cdr!");
    value = kmake_applicative(K, set_cdrB, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.7.2 copy-es-immutable */
    /* TODO */

    /*
    ** 4.8 Environments
    */

    /* 4.8.1 environment? */
    symbol = ksymbol_new(K, "environment?");
    value = kmake_applicative(K, environmentp, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.8.2 ignore? */
    symbol = ksymbol_new(K, "ignore?");
    value = kmake_applicative(K, ignorep, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.8.3 eval */
    symbol = ksymbol_new(K, "eval");
    value = kmake_applicative(K, eval, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.8.4 make-environment */
    symbol = ksymbol_new(K, "make-environment");
    value = kmake_applicative(K, make_environment, 0);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.9 Environment mutation
    */

    /* 4.9.1 $define! */
    symbol = ksymbol_new(K, "$define!");
    value = kmake_operative(K, KNIL, KNIL, SdefineB, 1, symbol);
    kadd_binding(K, ground_env, symbol, value);

    /*
    ** 4.10 Combiners
    */

    /* 4.10.1 operative? */
    symbol = ksymbol_new(K, "operative?");
    value = kmake_applicative(K, operativep, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.10.2 applicative? */
    symbol = ksymbol_new(K, "applicative?");
    value = kmake_applicative(K, applicativep, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.10.3 $vau */
    /* TODO */

    /* 4.10.4 wrap */
    symbol = ksymbol_new(K, "wrap");
    value = kmake_applicative(K, wrap, 0);
    kadd_binding(K, ground_env, symbol, value);

    /* 4.10.5 unwrap */
    symbol = ksymbol_new(K, "unwrap");
    value = kmake_applicative(K, unwrap, 0);
    kadd_binding(K, ground_env, symbol, value);

    return ground_env;
}
