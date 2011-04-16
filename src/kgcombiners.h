/*
** kgcombiners.h
** Combiners features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgcombiners_h
#define kgcombiners_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* 4.10.1 operative? */
/* uses typep */

/* 4.10.2 applicative? */
/* uses typep */

/* 4.10.3 $vau */
/* 5.3.1 $vau */
void Svau(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 4.10.4 wrap */
void wrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 4.10.5 unwrap */
void unwrap(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.3.1 $vau */
/* DONE: above, together with 4.10.4 */

/* 5.3.2 $lambda */
void Slambda(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 5.5.1 apply */
void apply(klisp_State *K, TValue *xparams, TValue ptree, 
	   TValue denv);

/* Helpers for map (also used by for each) */

/* Calculate the metrics for both the result list and the ptree
   passed to the applicative */
void map_for_each_get_metrics(
    klisp_State *K, char *name, TValue lss, int32_t *app_apairs_out, 
    int32_t *app_cpairs_out, int32_t *res_apairs_out, int32_t *res_cpairs_out);

/* Return two lists, isomorphic to lss: one list of cars and one list
   of cdrs (replacing the value of lss) */
/* GC: Assumes lss is rooted, uses dummys 2 & 3 */
TValue map_for_each_get_cars_cdrs(klisp_State *K, TValue *lss, 
				  int32_t apairs, int32_t cpairs);

/* Transpose lss so that the result is a list of lists, each one having
   metrics (app_apairs, app_cpairs). The metrics of the returned list
   should be (res_apairs, res_cpairs) */

/* GC: Assumes lss is rooted, uses dummys 1, & 
   (through get_cars_cdrs, 2, 3) */
TValue map_for_each_transpose(klisp_State *K, TValue lss, 
			      int32_t app_apairs, int32_t app_cpairs, 
			      int32_t res_apairs, int32_t res_cpairs);

/* 5.9.1 map */
void map(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);

/* 6.2.1 combiner? */
/* uses ftypedp */

/* Helper for combiner? */
bool kcombinerp(TValue obj);

#endif
