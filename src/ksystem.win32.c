/*
** ksystem.win32.c
** Platform dependent functionality - version for Windows.
** See Copyright Notice in klisp.h
*/

#include <windows.h>
#include <stdio.h>
#include "kobject.h"
#include "kstate.h"
#include "kinteger.h"
#include "kport.h"
#include "ksystem.h"

/* declare implemented functionality */

#define HAVE_PLATFORM_JIFFIES
#define HAVE_PLATFORM_ISATTY

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

bool ksystem_isatty(klisp_State *K, TValue port)
{
    if (!ttisfport(port) || kport_is_closed(port))
        return false;

    /* get the underlying Windows File HANDLE */

    int fd = _fileno(kfport_file(port));
    if (fd == -1 || fd == -2)
        return false;

    HANDLE h = (HANDLE) _get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE)
        return false;

    /* Googling gives two unreliable ways to emulate isatty():
     *
     *  1) test if GetFileType() returns FILE_TYPE_CHAR
     *    - reports NUL special file as a terminal
     *
     *  2) test if GetConsoleMode() succeeds
     *    - does not work on output handles
     *    - does not work in plain wine (works in wineconsole)
     *    - probably won't work if Windows Console is replaced
     *      a terminal emulator
     *
     * TEMP: use GetConsoleMode()
     */

    DWORD mode;
    return GetConsoleMode(h, &mode);
}
