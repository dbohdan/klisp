/*
** kgkd_vars.h
** Keyed Dynamic Variables features for the ground environment
** See Copyright Notice in klisp.h
*/

#ifndef kgkd_vars_h
#define kgkd_vars_h

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kobject.h"
#include "klisp.h"
#include "kstate.h"
#include "kghelpers.h"

/* This is also used by kgports.c */
void do_bind(klisp_State *K);
void do_access(klisp_State *K);

/* 10.1.1 make-keyed-dynamic-variable */
void make_keyed_dynamic_variable(klisp_State *K);

void do_unbind(klisp_State *K);

/* init ground */
void kinit_kgkd_vars_ground_env(klisp_State *K);

#endif
