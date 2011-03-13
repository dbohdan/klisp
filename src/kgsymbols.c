/*
** kgsymbols.c
** Symbol features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "klisp.h"
#include "kcontinuation.h"
#include "kpair.h"
#include "kstring.h"
#include "ksymbol.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgsymbols.h"

/* 4.4.1 symbol? */
/* uses typep */
