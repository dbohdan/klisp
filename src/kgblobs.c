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

    if (knegativep(K, tv_s)) {
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
	int s = ivalue(tv_s);
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

    /* 13.1.1? blob? */
    add_applicative(K, ground_env, "blob?", typep, 2, symbol, 
		    i2tv(K_TBLOB));
    /* 13.1.2? make-blob */
    add_applicative(K, ground_env, "make-blob", make_blob, 0);
    /* 13.1.3? blob-length */
    add_applicative(K, ground_env, "blob-length", blob_length, 0);

/* TODO put the blob equivalents here */
#if 0
    /* 13.1.4? string-ref */
    add_applicative(K, ground_env, "string-ref", string_ref, 0);
    /* 13.1.5? string-set! */
    add_applicative(K, ground_env, "string-set!", string_setS, 0);
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
