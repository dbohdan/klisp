/*
** klimits.h
** Limits, basic types, and some other `installation-dependent' definitions
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is from lua (greatly reduced)
*/

#ifndef klimits_h
#define klimits_h

#include <limits.h>
#include <stddef.h>

#include "klisp.h"

/* internal assertions for in-house debugging */
#ifdef klisp_assert

#define check_exp(c,e)		(klisp_assert(c), (e))

#else

#define klisp_assert(c)		((void)0)
#define check_exp(c,e)		(e)

#endif


#ifndef UNUSED
#define UNUSED(x)	((void)(x))	/* to avoid warnings */
#endif

#ifndef cast
#define cast(t, exp)	((t)(exp))
#endif

#endif
