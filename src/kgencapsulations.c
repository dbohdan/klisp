/*
** kgencapsulations.c
** Encapsulations features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kencapsulation.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgencapsulations.h"

/* Helpers for make-encapsulation-type */

/* Type predicate for encapsulations */
void enc_typep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
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

/* Constructor for encapsulations */
void enc_wrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    bind_1p(K, ptree, obj);
    UNUSED(denv);
    /*
    ** xparams[0]: encapsulation key
    */
    TValue key = xparams[0];
    TValue enc = kmake_encapsulation(K, key, obj);
    kapply_cc(K, enc);
}

/* Accessor for encapsulations */
void enc_unwrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    bind_1p(K, ptree, enc);
    UNUSED(denv);
    /*
    ** xparams[0]: encapsulation key
    */
    TValue key = xparams[0];

    if (!kis_encapsulation_type(enc, key)) {
	klispE_throw_simple(K, "object doesn't belong to this "
		     "encapsulation type");
	return;
    }
    TValue obj = kget_enc_val(enc);
    kapply_cc(K, obj);
}

/* 8.1.1 make-encapsulation-type */
void make_encapsulation_type(klisp_State *K, TValue *xparams, TValue ptree,
			     TValue denv)
{
    check_0p(K, ptree);
    UNUSED(denv);
    UNUSED(xparams);

    /* GC: root intermediate values & pairs */
    TValue key = kmake_encapsulation_key(K);
    krooted_tvs_push(K, key);
    TValue e = kmake_applicative(K, enc_wrap, 1, key);
    krooted_tvs_push(K, e);
    TValue p = kmake_applicative(K, enc_typep, 1, key);
    krooted_tvs_push(K, p);
    TValue d = kmake_applicative(K, enc_unwrap, 1, key);
    krooted_tvs_push(K, d);

    TValue ls = klist(K, 3, e, p, d);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, ls);
}
