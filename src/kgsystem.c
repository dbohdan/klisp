/*
** kgsystem.c
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgsystem.h"

/* ??.?.?  */

/* init ground */
void kinit_system_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

/* TODO */
#if 0
    /* 15.2.2 load */
    add_applicative(K, ground_env, "load", load, 0);
    /* 15.2.3 get-module */
    add_applicative(K, ground_env, "get-module", get_module, 0);
    /* 15.2.? display */
    add_applicative(K, ground_env, "display", display, 0);
#endif
}
