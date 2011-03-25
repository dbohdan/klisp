/*
** kgenv_mut.c
** Environment mutation features for the ground environment
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
#include "kenvironment.h"
#include "kcontinuation.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgenv_mut.h"

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

/* helper */
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

/* 6.8.1 $set! */
void SsetB(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);

    TValue sname = xparams[0];

    bind_3p(K, "$set!", ptree, env_exp, raw_formals, eval_exp);

    TValue formals = check_copy_ptree(K, "$set!", raw_formals, KIGNORE);

    TValue new_cont = 
	kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_set_eval_obj, 4, 
			   sname, formals, eval_exp, denv);
    kset_cc(K, new_cont);
    ktail_eval(K, env_exp, denv);
}

/* Helpers for $set! */
void do_set_eval_obj(klisp_State *K, TValue *xparams, TValue obj)
{
    TValue sname = xparams[0];
    TValue formals = xparams[1];
    TValue eval_exp = xparams[2];
    TValue denv = xparams[3];
    
    if (!ttisenvironment(obj)) {
	klispE_throw_extra(K, ksymbol_buf(sname), ": bad type from first "
			   "operand evaluation (expected environment)");
	return;
    } else {
	TValue env = obj;

	TValue new_cont = 
	    kmake_continuation(K, kget_cc(K), KNIL, KNIL, do_match, 3, 
			       formals, env, sname);
	kset_cc(K, new_cont);
	ktail_eval(K, eval_exp, denv);
    }
}

/* 6.8.2 $provide! */
/* TODO */

/* 6.8.3 $import! */
/* TODO */
