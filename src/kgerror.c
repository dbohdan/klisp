/*
** kgerror.c
** Error handling features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kstring.h"
#include "kpair.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgerror.h"

void r7rs_error(klisp_State *K, TValue *xparams, TValue ptree,
                TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    if (ttispair(ptree) && ttisstring(kcar(ptree))) {
        klispE_throw_with_irritants(K, kstring_buf(kcar(ptree)), kcdr(ptree));
    } else {
        klispE_throw_with_irritants(K, "Unknown error in user code", ptree);
    }
}

void error_object_message(klisp_State *K, TValue *xparams, TValue ptree,
                          TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "error object", ttiserror, error_tv);
    Error *err_obj = tv2error(error_tv);
    assert(ttisstring(err_obj->msg));
    kapply_cc(K, err_obj->msg);
}

void error_object_irritants(klisp_State *K, TValue *xparams, TValue ptree,
                          TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "error object", ttiserror, error_tv);
    Error *err_obj = tv2error(error_tv);
    kapply_cc(K, err_obj->irritants);
}

/* init ground */
void kinit_error_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    add_applicative(K, ground_env, "error-object?", typep, 2, symbol, i2tv(K_TERROR));
    add_applicative(K, ground_env, "error", r7rs_error, 0);
    add_applicative(K, ground_env, "error-object-message", error_object_message, 0);
    add_applicative(K, ground_env, "error-object-irritants", error_object_irritants, 0);
}
