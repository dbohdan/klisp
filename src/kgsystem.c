/*
** kgsystem.c
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgsystem.h"

/* 
** SOURCE NOTE: These are all from the r7rs draft.
*/

/* ??.?.?  current-second */
void current_second(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);
    time_t now = time(NULL);
    if (now == -1) {
	klispE_throw_simple(K, "couldn't get time");
	return;
    } else {
	if (now > INT32_MAX) {
	    /* XXX/TODO create bigint */
	    klispE_throw_simple(K, "integer too big");
	    return;
	} else {
	    kapply_cc(K, i2tv((int32_t) now));
	    return;
	}
    }
}

/* ??.?.?  current-jiffy */
void current_jiffy(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);
    /* TODO, this may wrap around... use time+clock to a better number */
    /* XXX doesn't seem to work... should probably use gettimeofday
       in posix anyways */
    clock_t now = clock();
    if (now == -1) {
	klispE_throw_simple(K, "couldn't get time");
	return;
    } else {
	if (now > INT32_MAX) {
	    /* XXX/TODO create bigint */
	    klispE_throw_simple(K, "integer too big");
	    return;
	} else {
	    kapply_cc(K, i2tv((int32_t) now));
	    return;
	}
    }
}

/* ??.?.?  jiffies-per-second */
void jiffies_per_second(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);
    if (CLOCKS_PER_SEC > INT32_MAX) {
	    /* XXX/TODO create bigint */
	    klispE_throw_simple(K, "integer too big");
	    return;
    } else {
	kapply_cc(K, i2tv((int32_t) CLOCKS_PER_SEC));
	return;
    }
}

/* init ground */
void kinit_system_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* ??.?.? current-second */
    add_applicative(K, ground_env, "current-second", current_second, 0);
    /* ??.?.? current-jiffy */
    add_applicative(K, ground_env, "current-jiffy", current_jiffy, 0);
    /* ??.?.? jiffies-per-second */
    add_applicative(K, ground_env, "jiffies-per-second", jiffies_per_second, 
		    0);
}
