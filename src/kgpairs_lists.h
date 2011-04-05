/*
** kgpairs_lists.h
** Pairs and lists features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgpairs_lists_h
#define kgpairs_lists_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.6.1 pair? */
/* uses typep */

/* 4.6.2 null? */
/* uses typep */
    
/* 4.6.3 cons */
void cons(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.2.1 list */
void list(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.2.2 list* */
void listS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.4.1 car, cdr */
/* 5.4.2 caar, cadr, ... cddddr */
void c_ad_r( klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* Helper macros to construct xparams[1] for c[ad]{1,4}r */
#define C_AD_R_PARAM(len_, br_) \
    (i2tv((C_AD_R_LEN(len_) | (C_AD_R_BRANCH(br_)))))
#define C_AD_R_LEN(len_) ((len_) << 4)
#define C_AD_R_BRANCH(br_) \
    ((br_ & 0x0001? 0x1 : 0) | \
     (br_ & 0x0010? 0x2 : 0) | \
     (br_ & 0x0100? 0x4 : 0) | \
     (br_ & 0x1000? 0x8 : 0))

/* 5.7.1 get-list-metrics */
void get_list_metrics(klisp_State *K, TValue *xparams, TValue ptree, 
		      TValue denv);

/* 5.7.2 list-tail */
void list_tail(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.1 length */
void length(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.2 list-ref */
void list_ref(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.3 append */
void append(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.4 list-neighbors */
void list_neighbors(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 6.3.5 filter */
void filter(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.6 assoc */
void assoc(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.7 member? */
void memberp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.8 finite-list? */
void finite_listp(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.3.9 countable-list? */
void countable_listp(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv);

/* 6.3.10 reduce */
/* TODO */

#endif
