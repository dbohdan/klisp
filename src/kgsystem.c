/*
** kgsystem.c
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgsystem.h"

/* 
** SOURCE NOTE: These are all from the r7rs draft.
*/

/* ??.?.?  current-second */
void current_second(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);
    time_t now = time(NULL);
    if (now == -1) {
	klispE_throw_simple(K, "couldn't get time");
	return;
    } else {
	if (now > INT32_MAX) {
	    /* XXX/TODO create bigint */
	    klispE_throw_simple(K, "integer too big");
	    return;
	} else {
	    kapply_cc(K, i2tv((int32_t) now));
	    return;
	}
    }
}

/* ??.?.?  current-jiffy */
void current_jiffy(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);
    /* TODO, this may wrap around... use time+clock to a better number */
    /* XXX doesn't seem to work... should probably use gettimeofday
       in posix anyways */
    clock_t now = clock();
    if (now == -1) {
	klispE_throw_simple(K, "couldn't get time");
	return;
    } else {
	if (now > INT32_MAX) {
	    /* XXX/TODO create bigint */
	    klispE_throw_simple(K, "integer too big");
	    return;
	} else {
	    kapply_cc(K, i2tv((int32_t) now));
	    return;
	}
    }
}

/* ??.?.?  jiffies-per-second */
void jiffies_per_second(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);
    if (CLOCKS_PER_SEC > INT32_MAX) {
	    /* XXX/TODO create bigint */
	    klispE_throw_simple(K, "integer too big");
	    return;
    } else {
	kapply_cc(K, i2tv((int32_t) CLOCKS_PER_SEC));
	return;
    }
}

/* 15.1.? file-exists? */
void file_existsp(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, filename);

    /* TEMP: this should probably be done in a operating system specific
       manner, but this will do for now */
    TValue res = KFALSE;
    FILE *file = fopen(kstring_buf(filename), "r");
    if (file) {
	res = KTRUE;
	UNUSED(fclose(file));
    }
    kapply_cc(K, res);
}

/* 15.1.? delete-file */
void delete_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, filename);

    /* TEMP: this should probably be done in a operating system specific
       manner, but this will do for now */
    /* XXX: this could fail if there's a dead (in the gc sense) port still 
       open, should probably retry once after doing a complete GC */
    if (remove(kstring_buf(filename))) {
        klispE_throw_errno_with_irritants(K, "remove", 1, filename);
        return;
    } else {
	kapply_cc(K, KINERT);
	return;
    }
}

/* 15.1.? rename-file */
void rename_file(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_2tp(K, ptree, "string", ttisstring, old_filename, 
	     "string", ttisstring, new_filename);

    /* TEMP: this should probably be done in a operating system specific
       manner, but this will do for now */
    /* XXX: this could fail if there's a dead (in the gc sense) port still 
       open, should probably retry once after doing a complete GC */
    if (rename(kstring_buf(old_filename), kstring_buf(new_filename))) {
        klispE_throw_errno_with_irritants(K, "rename", 2, old_filename, new_filename);
        return;
    } else {
	kapply_cc(K, KINERT);
	return;
    }
}

/* used for both get_script_arguments and get_interpreter_arguments */
void get_arguments(klisp_State *K)
{
    /*
     * xparams[0]: immutable argument list
     */
    TValue ptree = K->next_value;
    TValue *xparams = K->next_xparams;
    check_0p(K, ptree);

    TValue res = xparams[0];
    kapply_cc(K, res);
}

/* init ground */
void kinit_system_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* ??.?.? current-second */
    add_applicative(K, ground_env, "current-second", current_second, 0);
    /* ??.?.? current-jiffy */
    add_applicative(K, ground_env, "current-jiffy", current_jiffy, 0);
    /* ??.?.? jiffies-per-second */
    add_applicative(K, ground_env, "jiffies-per-second", jiffies_per_second, 
		    0);
    /* ?.? file-exists? */
    add_applicative(K, ground_env, "file-exists?", file_existsp, 0);
    /* ?.? delete-file */
    add_applicative(K, ground_env, "delete-file", delete_file, 0);
    /* this isn't in r7rs but it's in ansi c and quite easy to implement */
    /* ?.? rename-file */
    add_applicative(K, ground_env, "rename-file", rename_file, 0);
    /* The value for these two will get set later by the interpreter */
    /* ?.? get-script-arguments */
    add_applicative(K, ground_env, "get-script-arguments", get_arguments, 
		    1, KNIL);
    /* ?.? get-interpreter-arguments */
    add_applicative(K, ground_env, "get-interpreter-arguments", get_arguments, 
		    1, KNIL);
}
