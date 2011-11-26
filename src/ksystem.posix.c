/*
** ksystem.posix.c
** Platform dependent functionality - version for POSIX systems.
** See Copyright Notice in klisp.h
*/

#include <sys/time.h>
#include "kobject.h"
#include "kstate.h"
#include "kinteger.h"
#include "ksystem.h"

/* declare implemented functionality */

#define HAVE_PLATFORM_JIFFIES

/* jiffies */

TValue ksystem_current_jiffy(klisp_State *K)
{
    /* TEMP: use gettimeofday(). clock_gettime(CLOCK_MONOTONIC,...)
     * might be more apropriate, but it is reportedly not
     * supported on MacOS X. */

    struct timeval tv;
    gettimeofday(&tv, NULL);

    TValue res = kbigint_make_simple(K);
    krooted_vars_push(K, &res);
    mp_int_set_value(K, tv2bigint(res), tv.tv_sec);
    mp_int_mul_value(K, tv2bigint(res), 1000000, tv2bigint(res));
    mp_int_add_value(K, tv2bigint(res), tv.tv_usec, tv2bigint(res));
    krooted_vars_pop(K);

    return res;
}

TValue ksystem_jiffies_per_second(klisp_State *K)
{
    UNUSED(K);
    return i2tv(1000000);
}
