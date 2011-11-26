/*
** ksystem.win32.c
** Platform dependent functionality - version for Windows.
** See Copyright Notice in klisp.h
*/

#include <windows.h>
#include "kobject.h"
#include "kstate.h"
#include "kinteger.h"
#include "ksystem.h"

/* declare implemented functionality */

#define HAVE_PLATFORM_JIFFIES

/* jiffies */

TValue ksystem_current_jiffy(klisp_State *K)
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return kinteger_new_uint64(K, li.QuadPart);
}

TValue ksystem_jiffies_per_second(klisp_State *K)
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return kinteger_new_uint64(K, li.QuadPart);
}
