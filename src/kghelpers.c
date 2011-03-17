/*
** kghelpers.c
** Helper macros and functions for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "kghelpers.h"
#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kerror.h"
#include "ksymbol.h"

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

void ftypep(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    (void) denv;
    /*
    ** xparams[0]: name symbol
    ** xparams[1]: fn pointer (as a void * in a user TValue)
    */
    bool (*fn)(TValue obj) = pvalue(xparams[1]);

    /* check the ptree is a list while checking the predicate.
       Keep going even if the result is false to catch errors in 
       ptree structure */
    bool res = true;

    TValue tail = ptree;
    while(ttispair(tail) && kis_unmarked(tail)) {
	kmark(tail);
	res &= (*fn)(kcar(tail));
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
