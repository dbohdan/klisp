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

/* 13.1.2? make-bytevector */
void make_bytevector(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_al1tp(K, ptree, "exact integer", keintegerp, tv_s, 
	       maybe_byte);

    uint8_t fill = 0;
    if (get_opt_tpar(K, "make-bytevector", K_TFIXINT, &maybe_byte)) {
	if (ivalue(maybe_byte) < 0 || ivalue(maybe_byte) > 255) {
	    klispE_throw_simple(K, "bad fill byte");    
	    return;
	}
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
void bytevector_length(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "bytevector", ttisbytevector, bytevector);

    TValue res = i2tv(kbytevector_size(bytevector));
    kapply_cc(K, res);
}

/* 13.1.4? bytevector-u8-ref */
void bytevector_u8_ref(klisp_State *K, TValue *xparams, TValue ptree, 
		       TValue denv)
{
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
void bytevector_u8_setS(klisp_State *K, TValue *xparams, TValue ptree, 
			TValue denv)
{
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

/* TODO change bytevector constructors to string like constructors */

/* 13.2.8? bytevector-copy */
/* TEMP: at least for now this always returns mutable bytevectors */
void bytevector_copy(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv)
{
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


/* 13.2.9? bytevector->immutable-bytevector */
void bytevector_to_immutable_bytevector(klisp_State *K, TValue *xparams, 
					TValue ptree, TValue denv)
{
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
    /* ??.1.?? bytevector->immutable-bytevector */
    add_applicative(K, ground_env, "bytevector->immutable-bytevector", 
		    bytevector_to_immutable_bytevector, 0);

/* TODO put the bytevector equivalents here */
#if 0
    /* 13.2.1? string */
    add_applicative(K, ground_env, "string", string, 0);
    /* 13.2.2? string=?, string-ci=? */
    add_applicative(K, ground_env, "string=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_eqp));
    add_applicative(K, ground_env, "string-ci=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_eqp));
    /* 13.2.3? string<?, string<=?, string>?, string>=? */
    add_applicative(K, ground_env, "string<?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ltp));
    add_applicative(K, ground_env, "string<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_lep));
    add_applicative(K, ground_env, "string>?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_gtp));
    add_applicative(K, ground_env, "string>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_gep));
    /* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
    add_applicative(K, ground_env, "string-ci<?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_ltp));
    add_applicative(K, ground_env, "string-ci<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_lep));
    add_applicative(K, ground_env, "string-ci>?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_gtp));
    add_applicative(K, ground_env, "string-ci>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_gep));
    /* 13.2.5? substring */
    add_applicative(K, ground_env, "substring", substring, 0);
    /* 13.2.6? string-append */
    add_applicative(K, ground_env, "string-append", string_append, 0);
    /* 13.2.7? string->list, list->string */
    add_applicative(K, ground_env, "string->list", string_to_list, 0);
    add_applicative(K, ground_env, "list->string", list_to_string, 0);
    /* 13.2.8? string-copy */
    add_applicative(K, ground_env, "string-copy", string_copy, 0);
    /* 13.2.9? string->immutable-string */
    add_applicative(K, ground_env, "string->immutable-string", 
		    string_to_immutable_string, 0);

    /* TODO: add string-immutable? or general immutable? */
    /* TODO: add string-upcase and string-downcase like in r7rs-draft */

    /* 13.2.10? string-fill! */
    add_applicative(K, ground_env, "string-fill!", string_fillS, 0);
#endif
}
