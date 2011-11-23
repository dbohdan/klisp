/*
** kgbytevectors.c
** Bytevectors features for the ground environment
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
#include "kbytevector.h"

#include "kghelpers.h"
#include "kgbytevectors.h"
#include "kgnumbers.h" /* for keintegerp & knegativep */

/* 13.1.1? bytevector? */
/* uses typep */

/* 13.? immutable-bytevector?, mutable-bytevector? */
/* use ftypep */

/* 13.1.2? make-bytevector */
void make_bytevector(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_al1tp(K, ptree, "exact integer", keintegerp, tv_s, 
	       maybe_byte);

    uint8_t fill = 0;
    if (get_opt_tpar(K, maybe_byte, "u8", ttisu8)) {
	fill = ivalue(maybe_byte);
    }

    if (knegativep(tv_s)) {
	klispE_throw_simple(K, "negative size");    
	return;
    } else if (!ttisfixint(tv_s)) {
	klispE_throw_simple(K, "size is too big");    
	return;
    }
    TValue new_bytevector = kbytevector_new_sf(K, ivalue(tv_s), fill);
    kapply_cc(K, new_bytevector);
}

/* 13.1.3? bytevector-length */
void bytevector_length(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "bytevector", ttisbytevector, bytevector);

    TValue res = i2tv(kbytevector_size(bytevector));
    kapply_cc(K, res);
}

/* 13.1.4? bytevector-u8-ref */
void bytevector_u8_ref(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "bytevector", ttisbytevector, bytevector,
	     "exact integer", keintegerp, tv_i);

    if (!ttisfixint(tv_i)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    }
    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kbytevector_size(bytevector)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    }

    TValue res = i2tv(kbytevector_buf(bytevector)[i]);
    kapply_cc(K, res);
}

/* 13.1.5? bytevector-u8-set! */
void bytevector_u8_setS(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, ptree, "bytevector", ttisbytevector, bytevector,
	     "exact integer", keintegerp, tv_i, "u8", ttisu8, tv_byte);

    if (!ttisfixint(tv_i)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    } else if (kbytevector_immutablep(bytevector)) {
	klispE_throw_simple(K, "immutable bytevector");
	return;
    } 

    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kbytevector_size(bytevector)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    }

    kbytevector_buf(bytevector)[i] = (uint8_t) ivalue(tv_byte);
    kapply_cc(K, KINERT);
}

/* 13.2.8? bytevector-copy */
/* TEMP: at least for now this always returns mutable bytevectors */
void bytevector_copy(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "bytevector", ttisbytevector, bytevector);

    TValue new_bytevector;
    /* the if isn't strictly necessary but it's clearer this way */
    if (tv_equal(bytevector, K->empty_bytevector)) {
	new_bytevector = bytevector; 
    } else {
	new_bytevector = kbytevector_new_bs(K, kbytevector_buf(bytevector),
					    kbytevector_size(bytevector));
    }
    kapply_cc(K, new_bytevector);
}

/* 13.2.9? bytevector-copy! */
void bytevector_copyS(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "bytevector", ttisbytevector, bytevector1, 
	"bytevector", ttisbytevector, bytevector2);

    if (kbytevector_immutablep(bytevector2)) {
	klispE_throw_simple(K, "immutable destination bytevector");
	return;
    } else if (kbytevector_size(bytevector1) > kbytevector_size(bytevector2)) {
	klispE_throw_simple(K, "destination bytevector is too small");
	return;
    }

    if (!tv_equal(bytevector1, bytevector2) && 
	  !tv_equal(bytevector1, K->empty_bytevector)) {
	memcpy(kbytevector_buf(bytevector2),
	       kbytevector_buf(bytevector1),
	       kbytevector_size(bytevector1));
    }
    kapply_cc(K, KINERT);
}

/* 13.2.10? bytevector-copy-partial */
/* TEMP: at least for now this always returns mutable bytevectors */
void bytevector_copy_partial(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, ptree, "bytevector", ttisbytevector, bytevector,
	     "exact integer", keintegerp, tv_start,
	     "exact integer", keintegerp, tv_end);

    if (!ttisfixint(tv_start) || ivalue(tv_start) < 0 ||
	  ivalue(tv_start) > kbytevector_size(bytevector)) {
	/* TODO show index */
	klispE_throw_simple(K, "start index out of bounds");
	return;
    } 

    int32_t start = ivalue(tv_start);

    if (!ttisfixint(tv_end) || ivalue(tv_end) < 0 || 
	  ivalue(tv_end) > kbytevector_size(bytevector)) {
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
    TValue new_bytevector;
    /* the if isn't strictly necessary but it's clearer this way */
    if (size == 0) {
	new_bytevector = K->empty_bytevector;
    } else {
	new_bytevector = kbytevector_new_bs(K, kbytevector_buf(bytevector) 
					    + start, size);
    }
    kapply_cc(K, new_bytevector);
}

/* 13.2.11? bytevector-copy-partial! */
void bytevector_copy_partialS(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_al3tp(K, ptree, "bytevector", ttisbytevector, bytevector1, 
	       "exact integer", keintegerp, tv_start,
	       "exact integer", keintegerp, tv_end,
	       rest);

    /* XXX: this will send wrong error msgs (bad number of arg) */
    bind_2tp(K, rest, 
	     "bytevector", ttisbytevector, bytevector2, 
	     "exact integer", keintegerp, tv_start2);

    if (!ttisfixint(tv_start) || ivalue(tv_start) < 0 ||
	  ivalue(tv_start) > kbytevector_size(bytevector1)) {
	/* TODO show index */
	klispE_throw_simple(K, "start index out of bounds");
	return;
    } 

    int32_t start = ivalue(tv_start);

    if (!ttisfixint(tv_end) || ivalue(tv_end) < 0 || 
	  ivalue(tv_end) > kbytevector_size(bytevector1)) {
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

    if (kbytevector_immutablep(bytevector2)) {
	klispE_throw_simple(K, "immutable destination bytevector");
	return;
    }

    if (!ttisfixint(tv_start2) || ivalue(tv_start2) < 0 || 
	  ivalue(tv_start2) > kbytevector_size(bytevector2)) {
	klispE_throw_simple(K, "to index out of bounds");
	return;
    }

    int32_t start2 = ivalue(tv_start2);
    int64_t end2 = (int64_t) start2 + size;

    if ((end2 > INT32_MAX) || 
	(((int32_t) end2) > kbytevector_size(bytevector2))) {
	klispE_throw_simple(K, "not enough space in destination");
	return;
    }

    if (size > 0) {
	memcpy(kbytevector_buf(bytevector2) + start2,
	       kbytevector_buf(bytevector1) + start,
	       size);
    }
    kapply_cc(K, KINERT);
}

/* 13.2.12? bytevector->immutable-bytevector */
void bytevector_to_immutable_bytevector(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "bytevector", ttisbytevector, bytevector);

    TValue res_bytevector;
    if (kbytevector_immutablep(bytevector)) {
/* this includes the empty bytevector */
	res_bytevector = bytevector;
    } else {
	res_bytevector = kbytevector_new_bs_imm(K, kbytevector_buf(bytevector), 
						kbytevector_size(bytevector));
    }
    kapply_cc(K, res_bytevector);
}

/* init ground */
void kinit_bytevectors_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

   /*
    ** This section is not in the report. The bindings here are
    ** taken from the r7rs scheme draft and should not be considered standard. 
    ** They are provided in the meantime to allow programs to use byte vectors.
    */

    /* ??.1.1? bytevector? */
    add_applicative(K, ground_env, "bytevector?", typep, 2, symbol, 
		    i2tv(K_TBYTEVECTOR));
    /* ??.? immutable-bytevector?, mutable-bytevector? */
    add_applicative(K, ground_env, "immutable-bytevector?", ftypep, 2, symbol, 
		    p2tv(kimmutable_bytevectorp));
    add_applicative(K, ground_env, "mutable-bytevector?", ftypep, 2, symbol, 
		    p2tv(kmutable_bytevectorp));
    /* ??.1.2? make-bytevector */
    add_applicative(K, ground_env, "make-bytevector", make_bytevector, 0);
    /* ??.1.3? bytevector-length */
    add_applicative(K, ground_env, "bytevector-length", bytevector_length, 0);

    /* ??.1.4? bytevector-u8-ref */
    add_applicative(K, ground_env, "bytevector-u8-ref", bytevector_u8_ref, 0);
    /* ??.1.5? bytevector-u8-set! */
    add_applicative(K, ground_env, "bytevector-u8-set!", bytevector_u8_setS, 
		    0);

    /* ??.1.?? bytevector-copy */
    add_applicative(K, ground_env, "bytevector-copy", bytevector_copy, 0);
    /* ??.1.?? bytevector-copy! */
    add_applicative(K, ground_env, "bytevector-copy!", bytevector_copyS, 0);

    /* ??.1.?? bytevector-copy-partial */
    add_applicative(K, ground_env, "bytevector-copy-partial", 
		    bytevector_copy_partial, 0);
    /* ??.1.?? bytevector-copy-partial! */
    add_applicative(K, ground_env, "bytevector-copy-partial!", 
		    bytevector_copy_partialS, 0);

    /* ??.1.?? bytevector->immutable-bytevector */
    add_applicative(K, ground_env, "bytevector->immutable-bytevector", 
		    bytevector_to_immutable_bytevector, 0);

}
