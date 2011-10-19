/*
** kgblobs.c
** Blobs features for the ground environment
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
#include "kblob.h"

#include "kghelpers.h"
#include "kgblobs.h"
#include "kgnumbers.h" /* for keintegerp & knegativep */

/* 13.1.1? blob? */
/* uses typep */

/* 13.1.2? make-blob */
void make_blob(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_al1tp(K, ptree, "exact integer", keintegerp, tv_s, 
	       maybe_byte);

    uint8_t fill = 0;
    if (get_opt_tpar(K, "make-blob", K_TFIXINT, &maybe_byte)) {
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
/* XXX/TODO */
/*    TValue new_blob = kblob_new_sf(K, ivalue(tv_s), fill); */
    TValue new_blob = kblob_new(K, ivalue(tv_s));
    if (fill != 0) {
	int32_t s = ivalue(tv_s);
	uint8_t *ptr = kblob_buf(new_blob);
	while(s--)
	    *ptr++ = fill;
    }

    kapply_cc(K, new_blob);
}

/* 13.1.3? blob-length */
void blob_length(klisp_State *K, TValue *xparams, TValue ptree, 
		     TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "blob", ttisblob, blob);

    TValue res = i2tv(kblob_size(blob));
    kapply_cc(K, res);
}

/* 13.1.4? blob-u8-ref */
void blob_u8_ref(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_2tp(K, ptree, "blob", ttisblob, blob,
	     "exact integer", keintegerp, tv_i);

    if (!ttisfixint(tv_i)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    }
    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kblob_size(blob)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    }

    TValue res = i2tv(kblob_buf(blob)[i]);
    kapply_cc(K, res);
}

/* 13.1.5? blob-u8-set! */
void blob_u8_setS(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_3tp(K, ptree, "blob", ttisblob, blob,
	     "exact integer", keintegerp, tv_i, "exact integer", keintegerp, tv_byte);

    if (!ttisfixint(tv_i)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    } else if (kblob_immutablep(blob)) {
	klispE_throw_simple(K, "immutable blob");
	return;
    } else if (ivalue(tv_byte) < 0 || ivalue(tv_byte) > 255) {
	klispE_throw_simple(K, "bad byte");
	return;
    }

    int32_t i = ivalue(tv_i);
    
    if (i < 0 || i >= kblob_size(blob)) {
	/* TODO show index */
	klispE_throw_simple(K, "index out of bounds");
	return;
    }

    kblob_buf(blob)[i] = (uint8_t) ivalue(tv_byte);
    kapply_cc(K, KINERT);
}

/* TODO change blob constructors to string like constructors */

/* 13.2.8? blob-copy */
/* TEMP: at least for now this always returns mutable blobs */
void blob_copy(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "blob", ttisblob, blob);

    TValue new_blob;
    /* the if isn't strictly necessary but it's clearer this way */
    if (tv_equal(blob, K->empty_blob)) {
	new_blob = blob; 
    } else {
	new_blob = kblob_new(K, kblob_size(blob));
	memcpy(kblob_buf(new_blob), kblob_buf(blob), kblob_size(blob));
    }
    kapply_cc(K, new_blob);
}

/* 13.2.9? blob->immutable-blob */
void blob_to_immutable_blob(klisp_State *K, TValue *xparams, 
				TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "blob", ttisblob, blob);

    TValue res_blob;
    if (kblob_immutablep(blob)) {/* this includes the empty blob */
	res_blob = blob;
    } else {
	res_blob = kblob_new_imm(K, kblob_size(blob));
	memcpy(kblob_buf(res_blob), kblob_buf(blob), kblob_size(blob));
    }
    kapply_cc(K, res_blob);
}

/* init ground */
void kinit_blobs_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

   /*
    ** This section is not in the report. The bindings here are
    ** taken from the r7rs scheme draft and should not be considered standard. 
    ** They are provided in the meantime to allow programs to use byte vectors.
    */

    /* ??.1.1? blob? */
    add_applicative(K, ground_env, "blob?", typep, 2, symbol, 
		    i2tv(K_TBLOB));
    /* ??.1.2? make-blob */
    add_applicative(K, ground_env, "make-blob", make_blob, 0);
    /* ??.1.3? blob-length */
    add_applicative(K, ground_env, "blob-length", blob_length, 0);

    /* ??.1.4? blob-u8-ref */
    add_applicative(K, ground_env, "blob-u8-ref", blob_u8_ref, 0);
    /* ??.1.5? blob-u8-set! */
    add_applicative(K, ground_env, "blob-u8-set!", blob_u8_setS, 0);

    /* ??.1.?? blob-copy */
    add_applicative(K, ground_env, "blob-copy", blob_copy, 0);
    /* ??.1.?? blob->immutable-blob */
    add_applicative(K, ground_env, "blob->immutable-blob", blob_to_immutable_blob, 0);

/* TODO put the blob equivalents here */
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
