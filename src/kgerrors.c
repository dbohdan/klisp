/*
** kgerrors.c
** Error handling features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kstring.h"
#include "kpair.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgerrors.h"

void kgerror(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_al1tp(K, ptree, "string", ttisstring, str, rest);
    /* copy the list of irritants, to avoid modification later */
    /* also check that is a list! */
    TValue irritants = check_copy_list(K, "error", rest, false);
    krooted_tvs_push(K, irritants);
    /* the msg is implicitly copied here */
    klispE_throw_with_irritants(K, kstring_buf(str), irritants);
}

void kgraise(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1p(K, ptree, obj);
    kcall_cont(K, K->error_cont, obj);
}

void error_object_message(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "error object", ttiserror, error_tv);
    Error *err_obj = tv2error(error_tv);
    /* the string is immutable, no need to copy it */
    klisp_assert(ttisstring(err_obj->msg));
    kapply_cc(K, err_obj->msg);
}

void error_object_irritants(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "error object", ttiserror, error_tv);
    Error *err_obj = tv2error(error_tv);
    kapply_cc(K, err_obj->irritants);
}

/* REFACTOR this is the same as do_pass_value */
void do_exception_cont(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    UNUSED(xparams);
    /* Just pass error object to general error continuation. */
    kapply_cc(K, obj);
}

/* REFACTOR maybe this should be in kerror.c */
/* Create system-error-continuation. */
void kinit_error_hierarchy(klisp_State *K)
{
    klisp_assert(ttiscontinuation(K->error_cont));
    klisp_assert(ttisinert(K->system_error_cont));

    K->system_error_cont = kmake_continuation(K, K->error_cont, 
					      do_exception_cont, 0);
}

/* init ground */
void kinit_error_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    add_applicative(K, ground_env, "error-object?", typep, 2, symbol, 
		    i2tv(K_TERROR));
    add_applicative(K, ground_env, "error", kgerror, 0);
    add_applicative(K, ground_env, "raise", kgraise, 0);
    /* MAYBE add get- and remove object from these names */
    add_applicative(K, ground_env, "error-object-message", 
		    error_object_message, 0);
    add_applicative(K, ground_env, "error-object-irritants", 
		    error_object_irritants, 0);
    /* TODO raise-continuable from r7rs doesn't make sense in the Kernel 
       system of handling continuations.
       What we could have is a more sofisticated system
       of restarts, which would be added to an error object
       and would encapsulate continuations and descriptions of them. 
       It would be accessible with 
       error-object-restarts or something like that.
       See Common Lisp and mit scheme for examples
    */

    klisp_assert(ttiscontinuation(K->system_error_cont));
    add_value(K, ground_env, "system-error-continuation", K->system_error_cont);
}
