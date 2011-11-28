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
#include "kbytevector.h"

#include "kghelpers.h"
#include "kgvectors.h"

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

/* (R7RS 3rd draft 6.3.6) vector */
void vector(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    /* don't allow cycles */
    int32_t pairs;
    check_list(K, false, ptree, &pairs, NULL);
    TValue res = list_to_vector_h(K, ptree, pairs);
    kapply_cc(K, res);
}

/* (R7RS 3rd draft 6.3.6) list->vector */
void list_to_vector(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_1p(K, ptree, ls);
    /* don't allow cycles */
    int32_t pairs;
    check_list(K, false, ls, &pairs, NULL);
    TValue res = list_to_vector_h(K, ls, pairs);
    kapply_cc(K, res);
}

/* (R7RS 3rd draft 6.3.6) vector->list */
void vector_to_list(klisp_State *K)
{
    klisp_assert(ttisenvironment(K->next_env));

    TValue ptree = K->next_value;
    bind_1tp(K, ptree, "vector", ttisvector, v);

    TValue res = vector_to_list_h(K, v, NULL);
    kapply_cc(K, res);
}

/* 13.? bytevector->vector, vector->bytevector */
void bytevector_to_vector(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "bytevector", ttisbytevector, str);
    TValue res;

    if (kbytevector_emptyp(str)) {
	res = K->empty_vector;
    } else {
	uint32_t size = kbytevector_size(str);

	/* MAYBE add vector constructor without fill */
	/* no need to root this */
	res = kvector_new_sf(K, size, KINERT);
	uint8_t  *src = kbytevector_buf(str);
	TValue *dst = kvector_buf(res);
	while(size--) {
	    uint8_t u8 = *src++; /* not needed but just in case */
	    *dst++ = i2tv(u8);
	}
    }
    kapply_cc(K, res);
}

/* TEMP Only ASCII for now */
void vector_to_bytevector(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    
    bind_1tp(K, ptree, "vector", ttisvector, vec);
    TValue res;

    if (kvector_emptyp(vec)) {
	res = K->empty_bytevector;
    } else {
	uint32_t size = kvector_size(vec);

	res = kbytevector_new_s(K, size); /* no need to root this */
	TValue *src = kvector_buf(vec);
	uint8_t *dst = kbytevector_buf(res);
	while(size--) {
	    TValue tv = *src++;
	    if (!ttisu8(tv)) {
		klispE_throw_simple_with_irritants(K, "Non u8 object found", 
						   1, tv);
		return;
	    }
	    *dst++ = (uint8_t) ivalue(tv);
	}
    }
    kapply_cc(K, res);
}

/* 13.2.9? vector-copy! */
void vector_copyB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "vector", ttisvector, vector1, 
	"vector", ttisvector, vector2);

    if (kvector_immutablep(vector2)) {
	klispE_throw_simple(K, "immutable destination vector");
	return;
    } else if (kvector_size(vector1) > kvector_size(vector2)) {
	klispE_throw_simple(K, "destination vector is too small");
	return;
    }

    if (!tv_equal(vector1, vector2) && 
	  !tv_equal(vector1, K->empty_vector)) {
	memcpy(kvector_buf(vector2),
	       kvector_buf(vector1),
	       kvector_size(vector1) * sizeof(TValue));
    }
    kapply_cc(K, KINERT);
}

/* ?.? vector-copy-partial */
/* TEMP: at least for now this always returns mutable vectors */
void vector_copy_partial(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, ptree, "vector", ttisvector, vector,
	     "exact integer", keintegerp, tv_start,
	     "exact integer", keintegerp, tv_end);

    if (!ttisfixint(tv_start) || ivalue(tv_start) < 0 ||
	  ivalue(tv_start) > kvector_size(vector)) {
	/* TODO show index */
	klispE_throw_simple(K, "start index out of bounds");
	return;
    } 

    int32_t start = ivalue(tv_start);

    if (!ttisfixint(tv_end) || ivalue(tv_end) < 0 || 
	  ivalue(tv_end) > kvector_size(vector)) {
	klispE_throw_simple(K, "end index out of bounds");
	return;
    }

    int32_t end = ivalue(tv_end);

    if (start > end) {
	/* TODO show indexes */
	klispE_throw_simple(K, "end index is smaller than start index");
	return;
    }

    int32_t size = end - start;
    TValue new_vector;
    /* the if isn't strictly necessary but it's clearer this way */
    if (size == 0) {
	new_vector = K->empty_vector;
    } else {
	new_vector = kvector_new_bs_g(K, true, kvector_buf(vector) 
				      + start, size);
    }
    kapply_cc(K, new_vector);
}

/* ?.? vector-copy-partial! */
void vector_copy_partialB(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_al3tp(K, ptree, "vector", ttisvector, vector1, 
	       "exact integer", keintegerp, tv_start,
	       "exact integer", keintegerp, tv_end,
	       rest);

    /* XXX: this will send wrong error msgs (bad number of arg) */
    bind_2tp(K, rest, 
	     "vector", ttisvector, vector2, 
	     "exact integer", keintegerp, tv_start2);

    if (!ttisfixint(tv_start) || ivalue(tv_start) < 0 ||
	  ivalue(tv_start) > kvector_size(vector1)) {
	/* TODO show index */
	klispE_throw_simple(K, "start index out of bounds");
	return;
    } 

    int32_t start = ivalue(tv_start);

    if (!ttisfixint(tv_end) || ivalue(tv_end) < 0 || 
	  ivalue(tv_end) > kvector_size(vector1)) {
	klispE_throw_simple(K, "end index out of bounds");
	return;
    }

    int32_t end = ivalue(tv_end);

    if (start > end) {
	/* TODO show indexes */
	klispE_throw_simple(K, "end index is smaller than start index");
	return;
    }

    int32_t size = end - start;

    if (kvector_immutablep(vector2)) {
	klispE_throw_simple(K, "immutable destination vector");
	return;
    }

    if (!ttisfixint(tv_start2) || ivalue(tv_start2) < 0 || 
	  ivalue(tv_start2) > kvector_size(vector2)) {
	klispE_throw_simple(K, "to index out of bounds");
	return;
    }

    int32_t start2 = ivalue(tv_start2);
    int64_t end2 = (int64_t) start2 + size;

    if ((end2 > INT32_MAX) || 
	(((int32_t) end2) > kvector_size(vector2))) {
	klispE_throw_simple(K, "not enough space in destination");
	return;
    }

    if (size > 0) {
	memcpy(kvector_buf(vector2) + start2,
	       kvector_buf(vector1) + start,
	       size * sizeof(TValue));
    }
    kapply_cc(K, KINERT);
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

    /* ?.? vector-copy */
    add_applicative(K, ground_env, "vector-copy", vector_copy, 0);

    /* ?.? vector->bytevector, bytevector->vector */
    add_applicative(K, ground_env, "vector->bytevector", 
		    vector_to_bytevector, 0);
    add_applicative(K, ground_env, "bytevector->vector", 
		    bytevector_to_vector, 0);

    /* ?.? vector->string, string->vector */
    /* in kgstrings.c */

    /* ?.? vector-copy! */
    add_applicative(K, ground_env, "vector-copy!", vector_copyB, 0);

    /* ?.? vector-copy-partial */
    add_applicative(K, ground_env, "vector-copy-partial", 
		    vector_copy_partial, 0);
    /* ?.? vector-copy-partial! */
    add_applicative(K, ground_env, "vector-copy-partial!", 
		    vector_copy_partialB, 0);

    /* ?.? vector-fill! */
    add_applicative(K, ground_env, "vector-fill!", vector_fillB, 0);

    /* ?.? vector->immutable-vector */
    add_applicative(K, ground_env, "vector->immutable-vector",
		    vector_to_immutable_vector, 0);
}
