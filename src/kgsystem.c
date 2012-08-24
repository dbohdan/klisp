/*
** kgsystem.c
** Ports features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kerror.h"
#include "ksystem.h"
#include "kinteger.h"
#include "kgc.h"

#include "kghelpers.h"
#include "kgsystem.h"

/* 
** SOURCE NOTE: These are all from the r7rs draft.
*/

/* ??.?.?  current-second */
/* XXX current revision of the r7rs draft asks for tai seconds,
   I am sticking with UTC seconds for now, at least till the report is 
   ratified */
void get_current_second(klisp_State *K)
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
        TValue res = kinteger_new_uint64(K, (uint64_t) now);
        kapply_cc(K, res);
    }
}

/* ??.?.?  current-jiffy */
void get_current_jiffy(klisp_State *K)
{
    TValue ptree = K->next_value;
    check_0p(K, ptree);
    kapply_cc(K, ksystem_current_jiffy(K));
}

/* ??.?.?  jiffies-per-second */
void get_jiffies_per_second(klisp_State *K)
{
    TValue ptree = K->next_value;
    check_0p(K, ptree);
    kapply_cc(K, ksystem_jiffies_per_second(K));
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
    if (remove(kstring_buf(filename))) {
        /* XXX lock? */
        /* At least in Windows, this could have failed if there's a dead
           (in the gc sense) port still open, should retry once after 
           doing a complete GC. This isn't ideal but... */
        klispC_fullgc(K);
        if (remove(kstring_buf(filename))) {
            klispE_throw_errno_with_irritants(K, "remove", 1, filename);
            return;
        }
    }
    kapply_cc(K, KINERT);
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

/* ?.? get-script-arguments, get-interpreter-arguments */
void get_arguments(klisp_State *K)
{
    /*
    ** xparams[0]: immutable argument list
    */
    TValue ptree = K->next_value;
    TValue *xparams = K->next_xparams;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);

    check_0p(K, ptree);
    TValue res = xparams[0];
    kapply_cc(K, res);
}

/* ?.? defined-environment-variable?, get-environment-variable, 
   get-environment-variables */
void defined_environment_variableP(klisp_State *K)
{
    TValue ptree = K->next_value;
    TValue *xparams = K->next_xparams;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, name);
    char *str = getenv(kstring_buf(name));

    TValue res = (str == NULL)? KFALSE : KTRUE;
    kapply_cc(K, res);
}

void get_environment_variable(klisp_State *K)
{
    TValue ptree = K->next_value;
    TValue *xparams = K->next_xparams;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "string", ttisstring, name);
    char *str = getenv(kstring_buf(name));

    TValue res;
    if (str == NULL) {
        klispE_throw_simple_with_irritants(K, "undefined env var", 1, name);
        return;
    } else {
        res = kstring_new_b_imm(K, str);
    }
    kapply_cc(K, res);
}

void get_environment_variables(klisp_State *K)
{
    /*
    ** xparams[0]: immutable variable list
    */
    TValue ptree = K->next_value;
    TValue *xparams = K->next_xparams;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);

    check_0p(K, ptree);
    kapply_cc(K, xparams[0]);
}

/* This should work in mingw as well as gcc */
/* TODO test, if that doesn't work, try to find a way
   avoiding taking extra params in main */
/* I think it's defined in unistd, but it needs to have __USE_GNU 
   defined. The correct way to do that would be to define _GNU_SOURCE
   before including any system files... That's not so good for an 
   embeddable interpreter, but it could be done in the makefile I guess */
extern
#ifdef _WIN32
__declspec(dllimport)
#endif
char **environ;

/* Helper for get-environment-variables */
TValue create_env_var_list(klisp_State *K)
{
    /* no need for gc guarding in this context */
    TValue var_name, var_value;
    TValue tail = KNIL;
    
    /* This should work in mingw as well as gcc */
    /* TODO test, if that doesn't work, try to find a way
       avoiding taking extra params in main */
    for(char **env = environ; *env != NULL; ++env) {
        /* *env is of the form: "<name>=<value>", presumably, name can't have
           an equal sign! */
        char *eq = strchr(*env, '=');
        int name_len = eq - *env;
        klisp_assert(eq != NULL); /* shouldn't happen */
        var_name = kstring_new_bs_imm(K, *env, name_len);
        var_value = kstring_new_b_imm(K, *env + name_len + 1);
        TValue new_entry = kimm_cons(K, var_name, var_value);
        tail = kimm_cons(K, new_entry, tail);
    }
    return tail;
}

/* init ground */
void kinit_system_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /* ??.?.? get-current-second */
    add_applicative(K, ground_env, "get-current-second", get_current_second, 0);
    /* ??.?.? get-current-jiffy */
    add_applicative(K, ground_env, "get-current-jiffy", get_current_jiffy, 0);
    /* ??.?.? get-jiffies-per-second */
    add_applicative(K, ground_env, "get-jiffies-per-second", get_jiffies_per_second, 
                    0);
    /* ?.? file-exists? */
    add_applicative(K, ground_env, "file-exists?", file_existsp, 0);
    /* ?.? delete-file */
    add_applicative(K, ground_env, "delete-file", delete_file, 0);
    /* this isn't in r7rs but it's in ansi c and quite easy to implement */
    /* ?.? rename-file */
    add_applicative(K, ground_env, "rename-file", rename_file, 0);
    /* The value for these two will get set later by the interpreter */
    /* ?.? get-script-arguments, get-interpreter-arguments */
    add_applicative(K, ground_env, "get-script-arguments", get_arguments, 
                    1, KNIL);
    add_applicative(K, ground_env, "get-interpreter-arguments", get_arguments, 
                    1, KNIL);
    /* ?.? defined-environment-variable?, get-environment-variable, 
       get-environment-variables */
    add_applicative(K, ground_env, "defined-environment-variable?", 
                    defined_environment_variableP, 0);
    add_applicative(K, ground_env, "get-environment-variable", 
                    get_environment_variable, 0);
    add_applicative(K, ground_env, "get-environment-variables", 
                    get_environment_variables, 1, create_env_var_list(K));
}
