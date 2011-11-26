/*
** ksystem.c
** Platform dependent functionality.
** See Copyright Notice in klisp.h
*/

#include "kobject.h"
#include "kstate.h"
#include "kerror.h"
#include "ksystem.h"

/* detect platform
 * TODO: sync with klispconf.h and kgffi.c */

#if defined(KLISP_USE_POSIX)
#    define KLISP_PLATFORM_POSIX
#elif defined(_WIN32)
#    define KLISP_PLATFORM_WIN32
#endif

/* Include platform-dependent versions. The platform-dependent
 * code #defines macro HAVE_PLATFORM_<functionality>, if it
 * actually implements <functionality>.
 */

#if defined(KLISP_PLATFORM_POSIX)
#    include "ksystem.posix.c"
#elif defined(KLISP_PLATFORM_WIN32)
#    include "ksystem.win32.c"
#endif

/* Fall back to platform-independent versions if necessaty. */

#ifndef HAVE_PLATFORM_JIFFIES

#include <time.h>

TValue ksystem_current_jiffy(klisp_State *K)
{
    /* N.B. clock() returns an approximation of processor time
     * used by the program. We want wall clock time here. */

    clock_t now = clock();
    if (now == -1) {
        klispE_throw_simple(K, "couldn't get time");
        return KFALSE;
    } else {
        if (now > INT32_MAX) {
            klispE_throw_simple(K, "integer too big");
            return KFALSE;
        } else {
            return i2tv((int32_t) now);
        }
    }
}

TValue ksystem_jiffies_per_second(klisp_State *K)
{
    if (CLOCKS_PER_SEC > INT32_MAX) {
        /* XXX/TODO create bigint */
        klispE_throw_simple(K, "integer too big");
        return KFALSE;
    } else {
        return i2tv((int32_t) CLOCKS_PER_SEC);
    }
}

#endif