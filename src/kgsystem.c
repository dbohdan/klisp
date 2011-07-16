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

/* ??.?.?  current-second */
void current_second(klisp_State *K, TValue *xparams, TValue ptree, 
		    TValue denv)
{
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

/* init ground */
void kinit_system_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

/* TODO */
    /* ??.?.? current-second */
    add_applicative(K, ground_env, "current-second", current_second, 0);
#if 0
    /* 15.2.3 get-module */
    add_applicative(K, ground_env, "get-module", get_module, 0);
    /* 15.2.? display */
    add_applicative(K, ground_env, "display", display, 0);
#endif
}
