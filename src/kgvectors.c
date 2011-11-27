/*
** kgvectors.c
** Vector (heterogenous array) features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kvector.h"
#include "kpair.h"

#include "kghelpers.h"
#include "kgvectors.h"
#include "kgnumbers.h" /* for keintegerp & knegativep */

/* (R7RS 3rd draft 6.3.6) vector? */
/* uses typep */

/* ?.?.? immutable-vector?, mutable-vector? */
/* use ftypep */

/* (R7RS 3rd draft 6.3.6) make-vector */
void make_vector(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));
    TValue ptree = K->next_value;

    bind_al1tp(K, ptree, "exact integer", keintegerp, tv_s, fill);
    if (!get_opt_tpar(K, fill, "any", anytype))
        fill = KINERT;

    if (knegativep(tv_s)) {
        klispE_throw_simple(K, "negative vector length");
        return;
    } else if (!ttisfixint(tv_s)) {
        klispE_throw_simple(K, "vector length is too big");
        return;
    }
    TValue new_vector = (ivalue(tv_s) == 0)?
	K->empty_vector
        : kvector_new_sf(K, ivalue(tv_s), fill);
    kapply_cc(K, new_vector);
}

/* (R7RS 3rd draft 6.3.6) vector-length */
void vector_length(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));
    TValue ptree = K->next_value;

    bind_1tp(K, ptree, "vector", ttisvector, vector);

    TValue res = i2tv(kvector_size(vector));
    kapply_cc(K, res);
}

/* (R7RS 3rd draft 6.3.6) vector-ref */
void vector_ref(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_2tp(K, ptree, "vector", ttisvector, vector,
             "exact integer", keintegerp, tv_i);

    if (!ttisfixint(tv_i)) {
        klispE_throw_simple_with_irritants(K, "vector index out of bounds",
                                           1, tv_i);
        return;
    }
    int32_t i = ivalue(tv_i);
    if (i < 0 || i >= kvector_size(vector)) {
        klispE_throw_simple_with_irritants(K, "vector index out of bounds",
                                           1, tv_i);
        return;
    }
    kapply_cc(K, kvector_buf(vector)[i]);
}

/* (R7RS 3rd draft 6.3.6) vector-set! */
void vector_setB(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_3tp(K, ptree, "vector", ttisvector, vector,
             "exact integer", keintegerp, tv_i, "any", anytype, tv_new_value);

    if (!ttisfixint(tv_i)) {
        klispE_throw_simple_with_irritants(K, "vector index out of bounds",
                                           1, tv_i);
        return;
    }

    int32_t i = ivalue(tv_i);
    if (i < 0 || i >= kvector_size(vector)) {
        klispE_throw_simple_with_irritants(K, "vector index out of bounds",
                                           1, tv_i);
        return;
    } else if (kvector_immutablep(vector)) {
        klispE_throw_simple(K, "immutable vector");
        return;
    }

    kvector_buf(vector)[i] = tv_new_value;
    kapply_cc(K, KINERT);
}

/* (R7RS 3rd draft 6.3.6) vector-copy */
/* TEMP: at least for now this always returns mutable vectors */
void vector_copy(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));
    TValue ptree = K->next_value;

    bind_1tp(K, ptree, "vector", ttisvector, v);

    TValue new_vector = kvector_emptyp(v)? 
	v
        : kvector_new_bs_g(K, true, kvector_buf(v), kvector_size(v));
    kapply_cc(K, new_vector);
}

static TValue list_to_vector_h(klisp_State *K, const char *name, TValue ls)
{
    int32_t dummy;
    int32_t pairs = check_list(K, name, false, ls, &dummy);

    if (pairs == 0) {
        return K->empty_vector;
    } else {
        TValue res = kvector_new_sf(K, pairs, KINERT);
        for (int i = 0; i < pairs; i++) {
            kvector_buf(res)[i] = kcar(ls);
            ls = kcdr(ls);
        }
        return res;
    }
}

/* (R7RS 3rd draft 6.3.6) vector */
void vector(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    TValue res = list_to_vector_h(K, "vector", ptree);
    kapply_cc(K, res);
}

/* (R7RS 3rd draft 6.3.6) list->vector */
void list_to_vector(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_1p(K, ptree, ls);
    TValue res = list_to_vector_h(K, "list->vector", ls);
    kapply_cc(K, res);
}

/* (R7RS 3rd draft 6.3.6) vector->list */
void vector_to_list(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_1tp(K, ptree, "vector", ttisvector, v);

    TValue tail = KNIL;
    krooted_vars_push(K, &tail);
    size_t i = kvector_size(v);
    while (i-- > 0)
        tail = kcons(K, kvector_buf(v)[i], tail);
    krooted_vars_pop(K);
    kapply_cc(K, tail);
}

/* ?.? vector-fill! */
void vector_fillB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "vector", ttisvector, vector,
	     "any", anytype, fill);

    if (kvector_immutablep(vector)) {
	klispE_throw_simple(K, "immutable vector");
	return;
    } 

    uint32_t size = kvector_size(vector);
    TValue *buf = kvector_buf(vector);
    while(size-- > 0) {
	*buf++ = fill;
    }
    kapply_cc(K, KINERT);
}

/* ??.?.? vector->immutable-vector */
void vector_to_immutable_vector(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_1tp(K, ptree, "vector", ttisvector, v);

    TValue res = kvector_immutablep(v)? 
	v
	: kvector_new_bs_g(K, false, kvector_buf(v), kvector_size(v));
    kapply_cc(K, res);
}

/* init ground */
void kinit_vectors_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

   /*
    ** This section is not in the report. The bindings here are
    ** taken from the r7rs scheme draft and should not be considered standard.
    ** They are provided in the meantime to allow programs to use vectors.
    */

    /* (R7RS 3rd draft 6.3.6) vector? */
    add_applicative(K, ground_env, "vector?", typep, 2, symbol,
                    i2tv(K_TVECTOR));
    /* ??.? immutable-vector?, mutable-vector? */
    add_applicative(K, ground_env, "immutable-vector?", ftypep, 2, symbol,
                    p2tv(kimmutable_vectorp));
    add_applicative(K, ground_env, "mutable-vector?", ftypep, 2, symbol,
                    p2tv(kmutable_vectorp));
    /* (R7RS 3rd draft 6.3.6) make-vector */
    add_applicative(K, ground_env, "make-vector", make_vector, 0);
    /* (R7RS 3rd draft 6.3.6) vector-length */
    add_applicative(K, ground_env, "vector-length", vector_length, 0);

    /* (R7RS 3rd draft 6.3.6) vector-ref vector-set! */
    add_applicative(K, ground_env, "vector-ref", vector_ref, 0);
    add_applicative(K, ground_env, "vector-set!", vector_setB, 0);

    /* (R7RS 3rd draft 6.3.6) vector, vector->list, list->vector */
    add_applicative(K, ground_env, "vector", vector, 0);
    add_applicative(K, ground_env, "vector->list", vector_to_list, 0);
    add_applicative(K, ground_env, "list->vector", list_to_vector, 0);

    /* ??.1.?? vector-copy */
    add_applicative(K, ground_env, "vector-copy", vector_copy, 0);

    /* TODO: vector->string, string->vector */
    /* TODO: vector-copy! vector-copy-partial vector-copy-partial! */

    add_applicative(K, ground_env, "vector-fill!", vector_fillB, 0);

    /* ??.1.?? vector->immutable-vector */
    add_applicative(K, ground_env, "vector->immutable-vector",
		    vector_to_immutable_vector, 0);
}
