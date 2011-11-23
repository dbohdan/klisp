/*
** kgsystem.h
** System features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgsystem_h
#define kgsystem_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* ??.?.? current-second */
void current_second(klisp_State *K);
/* ??.?.?  current-jiffy */
void current_jiffy(klisp_State *K);

/* init ground */
void kinit_system_ground_env(klisp_State *K);

#endif
