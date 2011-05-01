/*
** kreal.c
** Kernel Reals (doubles)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h> 
#include <math.h>

#include "kreal.h"
#include "krational.h"
#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

TValue kexact_to_inexact(klisp_State *K, TValue n)
{
    switch(ttype(n)) {
    case K_TFIXINT:
    case K_TBIGINT:
    case K_TBIGRAT:
    case K_TEINF:
	/* TODO */
	klisp_assert(0);
	/* all of these are already inexact */
    case K_TDOUBLE:
    case K_TIINF:
    case K_TRWNPV:
    case K_TUNDEFINED:
	return n;
    default:
	klisp_assert(0);
	return KINERT;
    }
}
