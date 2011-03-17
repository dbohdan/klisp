/*
** kgcontrol.c
** Control features for the ground environment
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
#include "kcontinuation.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgcontrol.h"

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
	/* TODO this could be at least in an inlineable function to
	   allow used from $lambda, $vau, $let family, load, etc */
	TValue tail = kcdr(ls);
	if (ttispair(tail)) {
	    TValue new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
					     do_seq, 2, tail, denv);
	    kset_cc(K, new_cont);
	} 
	ktail_eval(K, kcar(ls), denv);
    }
}

/* Helper (also used by $vau and $lambda) */
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

/* 5.6.1 $cond */
/* TODO */
