/*
** kgffi.c
** Foreign function interface
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <dlfcn.h>
#include <ffi.h>

#include "imath.h"
#include "kstate.h"
#include "kobject.h"
#include "kinteger.h"
#include "kpair.h"
#include "kerror.h"
#include "kblob.h"
#include "kencapsulation.h"

#include "kghelpers.h"
#include "kgencapsulations.h"
#include "kgffi.h"

/* Set to 0 to ignore aligment errors during direct
 * memory read/writes. */

#define KGFFI_CHECK_ALIGNMENT 1

typedef struct ffi_codec_s ffi_codec_t;
struct ffi_codec_s {
    const char *name;
    ffi_type *libffi_type;
    TValue (*decode)(ffi_codec_t *self, klisp_State *K, const void *buf);
    void (*encode)(ffi_codec_t *self, klisp_State *K, TValue v, void *buf);
};

typedef struct {
    ffi_cif cif;
    size_t buffer_size;
    ffi_codec_t *rcodec;
    size_t nargs;
    ffi_type **argtypes;
    ffi_codec_t **acodecs;
} ffi_call_interface_t;

static TValue ffi_decode_void(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    UNUSED(K);
    UNUSED(buf);
    return KINERT;
}

static TValue ffi_decode_sint(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    UNUSED(K);
    return i2tv(* (int *) buf);
}

static void ffi_encode_sint(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    if (!ttisfixint(v)) {
        klispE_throw_simple_with_irritants(K, "unable to convert to C int", 1, v);
        return;
    }
    /* TODO: bigint, ... */
    * (int *) buf = ivalue(v);
}

static TValue ffi_decode_pointer(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    void *p = *(void **)buf;
    return (p) ? p2tv(p) : KNIL;
}

static void ffi_encode_pointer(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    if (ttisblob(v)) {
        *(void **)buf = tv2blob(v)->b;
    } else if (ttisstring(v)) {
        *(void **)buf = kstring_buf(v);
    } else if (ttisnil(v)) {
        *(void **)buf = NULL;
    } else if (tbasetype_(v) == K_TAG_USER) {
       /* TODO: do not use internal macro tbasetype_ */
        *(void **)buf = pvalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "neither blob, string, pointer or nil", 1, v);
    }
}

static TValue ffi_decode_string(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    char *s = *(char **) buf;
    return (s) ? kstring_new_b_imm(K, s) : KNIL;
}

static void ffi_encode_string(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    if (ttisstring(v)) {
        *(void **)buf = kstring_buf(v);
    } else {
        klispE_throw_simple_with_irritants(K, "not a string", 1, v);
    }
}

static TValue ffi_decode_uint8(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    UNUSED(K);
    return i2tv(*(uint8_t *)buf);
}

static void ffi_encode_uint8(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    if (ttisfixint(v) && 0 <= ivalue(v) && ivalue(v) <= UINT8_MAX) {
        *(uint8_t *) buf = ivalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "unable to convert to C uint8_t", 1, v);
        return;
    }
}

static TValue ffi_decode_sint8(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    UNUSED(K);
    return i2tv(*(int8_t *)buf);
}

static void ffi_encode_sint8(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    if (ttisfixint(v) && INT8_MIN <= ivalue(v) && ivalue(v) <= INT8_MAX) {
        *(int8_t *) buf = ivalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "unable to convert to C int8_t", 1, v);
        return;
    }
}

static TValue ffi_decode_uint16(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    return i2tv(*(uint16_t *)buf);
}

static void ffi_encode_uint16(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    if (ttisfixint(v) && 0 <= ivalue(v) && ivalue(v) <= UINT16_MAX) {
        *(uint16_t *) buf = ivalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "unable to convert to C uint16_t", 1, v);
        return;
    }
}

static TValue ffi_decode_sint16(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    return i2tv(*(int16_t *)buf);
}

static void ffi_encode_sint16(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    if (ttisfixint(v) && INT16_MIN <= ivalue(v) && ivalue(v) <= INT16_MAX) {
        *(int16_t *) buf = ivalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "unable to convert to C int16_t", 1, v);
        return;
    }
}

static TValue ffi_decode_uint32(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    uint32_t x = *(uint32_t *)buf;
    if (x <= INT32_MAX) {
        return i2tv((int32_t) x);
    } else {
        TValue res = kbigint_make_simple(K);
        krooted_tvs_push(K, res);

        uint8_t d[4];
        for (int i = 3; i >= 0; i--) {
          d[i] = (x & 0xFF);
          x >>= 8;
        }
        mp_int_read_unsigned(K, tv2bigint(res), d, 4);

        krooted_tvs_pop(K);
        return res;
    }
}

static void ffi_encode_uint32(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    uint32_t tmp;

    if (ttisfixint(v) && 0 <= ivalue(v)) {
        *(uint32_t *) buf = ivalue(v);
    } else if (ttisbigint(v) && mp_int_to_uint(tv2bigint(v), &tmp) == MP_OK) {
        *(uint32_t *) buf = tmp;
    } else {
        klispE_throw_simple_with_irritants(K, "unable to convert to C uint32_t", 1, v);
        return;
    }
}

static TValue ffi_decode_uint64(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    /* TODO */
    UNUSED(self);
    uint64_t x = *(uint64_t *)buf;
    if (x <= INT32_MAX) {
        return i2tv((int32_t) x);
    } else {
        TValue res = kbigint_make_simple(K);
        krooted_tvs_push(K, res);

        uint8_t d[8];
        for (int i = 7; i >= 0; i--) {
          d[i] = (x & 0xFF);
          x >>= 8;
        }

        mp_int_read_unsigned(K, tv2bigint(res), d, 8);
        krooted_tvs_pop(K);
        return res;
    }
}

static void ffi_encode_uint64(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    /* TODO */
    UNUSED(self);

    if (ttisfixint(v) && 0 <= ivalue(v)) {
        *(uint64_t *) buf = ivalue(v);
    } else if (ttisbigint(v)
              && mp_int_compare_zero(tv2bigint(v)) >= 0
              && mp_int_unsigned_len(tv2bigint(v)) <= 8) {
        uint8_t d[8];

        mp_int_to_unsigned(K, tv2bigint(v), d, 8);
        uint64_t tmp = d[0];
        for (int i = 1; i < 8; i++)
          tmp = (tmp << 8) | d[i];
        *(uint64_t *) buf = tmp;
    } else {
        klispE_throw_simple_with_irritants(K, "unable to convert to C uint64_t", 1, v);
        return;
    }
}

static TValue ffi_decode_double(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    return d2tv(*(double *)buf);
}

static void ffi_encode_double(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    if (ttisdouble(v)) {
        *(double *) buf = dvalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "unable to cast to C double", 1, v);
        return;
    }
}

static TValue ffi_decode_float(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    return d2tv((double) *(float *)buf);
}

static void ffi_encode_float(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    UNUSED(self);
    if (ttisdouble(v)) {
        /* TODO: avoid double rounding for rationals/bigints ?*/
        *(float *) buf = dvalue(v);
    } else {
        klispE_throw_simple_with_irritants(K, "unable to cast to C float", 1, v);
        return;
    }
}

static ffi_codec_t ffi_codecs[] = {
    { "void", &ffi_type_void, ffi_decode_void, NULL },
    { "string", &ffi_type_pointer, ffi_decode_string, ffi_encode_string },
#define SIMPLE_TYPE(t) { #t, &ffi_type_ ## t, ffi_decode_ ## t, ffi_encode_ ## t }
    SIMPLE_TYPE(sint),
    SIMPLE_TYPE(pointer),
    SIMPLE_TYPE(uint8),
    SIMPLE_TYPE(sint8),
    SIMPLE_TYPE(uint16),
    SIMPLE_TYPE(sint16),
    SIMPLE_TYPE(uint32),
    SIMPLE_TYPE(uint64),
    SIMPLE_TYPE(float),
    SIMPLE_TYPE(double)
#undef SIMPLE_TYPE
};

void ffi_load_library(klisp_State *K, TValue *xparams,
                      TValue ptree, TValue denv)
{
    UNUSED(denv);
    /*
    ** xparams[0]: encapsulation key denoting loaded library
    */

    TValue filename = ptree;
    const char *filename_c =
       get_opt_tpar(K, "ffi-load-library", K_TSTRING, &filename)
           ? kstring_buf(filename) : NULL;

    void *handle = dlopen(filename_c, RTLD_LAZY | RTLD_GLOBAL);
    if (handle == NULL) {
        krooted_tvs_push(K, filename);
        const char *err_c = dlerror();
        TValue err = (err_c == NULL) ? KNIL : kstring_new_b_imm(K, err_c);
        klispE_throw_simple_with_irritants(K, "couldn't load dynamic library",
                                           2, filename, err);
        return;
    }

    TValue key = xparams[0];
    krooted_tvs_push(K, key);

    TValue safe_filename = (filename_c) ? filename : kstring_new_b_imm(K, "interpreter binary or statically linked library");
    krooted_tvs_push(K, safe_filename);

    TValue lib_tv = kcons(K, p2tv(handle), safe_filename);
    krooted_tvs_push(K, lib_tv);

    TValue enc = kmake_encapsulation(K, key, lib_tv);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    kapply_cc(K, enc);
}

static ffi_abi tv2ffi_abi(klisp_State *K, TValue v)
{
    if (!strcmp("FFI_DEFAULT_ABI", kstring_buf(v))) {
        return FFI_DEFAULT_ABI;
    } else {
        klispE_throw_simple_with_irritants(K, "unsupported FFI ABI", 1, v);
        return 0;
    }
}

static ffi_codec_t *tv2ffi_codec(klisp_State *K, TValue v)
{
  for (size_t i = 0; i < sizeof(ffi_codecs)/sizeof(ffi_codecs[0]); i++) {
      if (!strcmp(ffi_codecs[i].name, kstring_buf(v)))
          return &ffi_codecs[i];
  }
  klispE_throw_simple_with_irritants(K, "unsupported FFI type", 1, v);
  return NULL;
}

static inline size_t align(size_t offset, size_t alignment)
{
    assert(alignment > 0);
    return offset + (alignment - offset % alignment) % alignment;
}

void ffi_make_call_interface(klisp_State *K, TValue *xparams,
                             TValue ptree, TValue denv)
{
    UNUSED(denv);
    /*
    ** xparams[0]: encapsulation key denoting call interface
    */

#define ttislist(v) (ttispair(v) || ttisnil(v))
    bind_3tp(K, ptree,
            "abi string", ttisstring, abi_tv,
            "rtype string", ttisstring, rtype_tv,
            "argtypes string list", ttislist, argtypes_tv);
#undef ttislist

    size_t nargs = check_typed_list(K, "ffi-make-call-interface", "argtype string",
                                    kstringp, false, argtypes_tv, NULL);

    krooted_tvs_push(K, abi_tv);
    krooted_tvs_push(K, rtype_tv);
    krooted_tvs_push(K, argtypes_tv);
    TValue key = xparams[0];
    krooted_tvs_push(K, key);
    size_t blob_size = sizeof(ffi_call_interface_t) + (sizeof(ffi_codec_t *) + sizeof(ffi_type)) * nargs;
    TValue blob = kblob_new_imm(K, blob_size);
    krooted_tvs_push(K, blob);
    TValue enc = kmake_encapsulation(K, key, blob);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    ffi_call_interface_t *p = (ffi_call_interface_t *) tv2blob(blob)->b;
    p->acodecs = (ffi_codec_t **) ((char *) p + sizeof(ffi_call_interface_t));
    p->argtypes = (ffi_type **) ((char *) p + sizeof(ffi_call_interface_t) + nargs * sizeof(ffi_codec_t *));

    p->nargs = nargs;
    p->rcodec = tv2ffi_codec(K, rtype_tv);
    if (p->rcodec->decode == NULL) {
      klispE_throw_simple(K, "this type is not allowed as a return type");
      return;
    }

    p->buffer_size = p->rcodec->libffi_type->size;
    TValue tail = argtypes_tv;
    for (int i = 0; i < nargs; i++) {
        p->acodecs[i] = tv2ffi_codec(K, kcar(tail));
        if (p->acodecs[i]->encode == NULL) {
            klispE_throw_simple(K, "this type is not allowed in argument list");
            return;
        }
        ffi_type *t = p->acodecs[i]->libffi_type;
        p->argtypes[i] = t;
        p->buffer_size = align(p->buffer_size, t->alignment) + t->size;
        tail = kcdr(tail);
    }
    ffi_abi abi = tv2ffi_abi(K, abi_tv);

    ffi_status status = ffi_prep_cif(&p->cif, abi, nargs, p->rcodec->libffi_type, p->argtypes);
    switch (status) {
        case FFI_OK:
            break;
        case FFI_BAD_ABI:
            klispE_throw_simple(K, "FFI_BAD_ABI");
            return;
        case FFI_BAD_TYPEDEF:
            klispE_throw_simple(K, "FFI_BAD_TYPEDEF");
            return;
        default:
            klispE_throw_simple(K, "unknown error in ffi_prep_cif");
            return;
    }
    kapply_cc(K, enc);
}

void do_ffi_call(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    /*
    ** xparams[0]: function pointer
    ** xparams[1]: call interface (encapsulated blob)
    */

    void *funptr = pvalue(xparams[0]);
    ffi_call_interface_t *p = (ffi_call_interface_t *) tv2blob(kget_enc_val(xparams[1]))->b;


    int64_t buffer[(p->buffer_size + sizeof(int64_t) - 1) / sizeof(int64_t)];
    void *aptrs[p->nargs];

    size_t offset = 0;
    void *rptr = (unsigned char *) buffer + offset;
    offset += p->rcodec->libffi_type->size;

    TValue tail = ptree;
    for (int i = 0; i < p->nargs; i++) {
        if (!ttispair(tail)) {
            klispE_throw_simple(K, "too few arguments");
            return;
        }
        ffi_type *t = p->acodecs[i]->libffi_type;
        offset = align(offset, t->alignment);
        aptrs[i] = (unsigned char *) buffer + offset;
        p->acodecs[i]->encode(p->acodecs[i], K, kcar(tail), aptrs[i]);
        offset += t->size;
        tail = kcdr(tail);
    }
    assert(offset == p->buffer_size);
    if (!ttisnil(tail)) {
        klispE_throw_simple(K, "too much arguments");
        return;
    }

    ffi_call(&p->cif, funptr, rptr, aptrs);

    TValue result = p->rcodec->decode(p->rcodec, K, rptr);
    kapply_cc(K, result);
}

void ffi_make_applicative(klisp_State *K, TValue *xparams,
                          TValue ptree, TValue denv)
{
    UNUSED(denv);
    /*
    ** xparams[0]: encapsulation key denoting dynamically loaded library
    ** xparams[1]: encapsulation key denoting call interface
    */

    bind_3tp(K, ptree,
            "dynamic library", ttisencapsulation, lib_tv,
            "function name string", ttisstring, name_tv,
            "call interface", ttisencapsulation, cif_tv);
    if (!kis_encapsulation_type(lib_tv, xparams[0])) {
        klispE_throw_simple(K, "first argument shall be dynamic library");
        return;
    }
    if (!kis_encapsulation_type(cif_tv, xparams[1])) {
        klispE_throw_simple(K, "third argument shall be call interface");
        return;
    }

    void *handle = pvalue(kcar(kget_enc_val(lib_tv)));
    TValue lib_name = kcdr(kget_enc_val(lib_tv));
    assert(ttisstring(lib_name));

    (void) dlerror();
    void *funptr = dlsym(handle, kstring_buf(name_tv));
    const char *err_c = dlerror();

    if (err_c) {
        krooted_tvs_push(K, name_tv);
        krooted_tvs_push(K, lib_name);
        TValue err = kstring_new_b_imm(K, err_c);
        klispE_throw_simple_with_irritants(K, "couldn't find symbol",
                                           3, lib_name, name_tv, err);
        return;
    }
    if (!funptr) {
        klispE_throw_simple_with_irritants(K, "symbol is NULL", 2,
                                           lib_name, name_tv);
    }

    TValue app = kmake_applicative(K, do_ffi_call, 2, p2tv(funptr), cif_tv);

#if KTRACK_SI
    krooted_tvs_push(K, app);
    krooted_tvs_push(K, lib_name);
    TValue tail = kcons(K, i2tv((int) funptr), i2tv(0));
    krooted_tvs_push(K, tail);
    TValue si = kcons(K, lib_name, tail);
    krooted_tvs_push(K, si);
    kset_source_info(K, kunwrap(app), si);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
#endif

    kapply_cc(K, app);
}

static uint8_t * ffi_memory_location(klisp_State *K, bool allow_nesting,
                                     TValue v, bool mutable, size_t size)
{
    if (ttisblob(v)) {
        if (mutable && kblob_immutablep(v)) {
            klispE_throw_simple_with_irritants(K, "blob not mutable", 1, v);
            return NULL;
        } else if (size > kblob_size(v)) {
            klispE_throw_simple_with_irritants(K, "blob too small", 1, v);
            return NULL;
        } else {
            return kblob_buf(v);
        }
    } else if (ttisstring(v)) {
        if (mutable && kstring_immutablep(v)) {
            klispE_throw_simple_with_irritants(K, "string not mutable", 1, v);
            return NULL;
        } else if (size > kstring_size(v)) {
            klispE_throw_simple_with_irritants(K, "string too small", 1, v);
            return NULL;
        } else {
            return (uint8_t *) kstring_buf(v);
        }
    } else if (tbasetype_(v) == K_TAG_USER) {
        /* TODO: do not use internal macro tbasetype_ */
        return (pvalue(v));
    } else if (ttispair(v) && ttispair(kcdr(v)) && ttisnil(kcddr(v))) {
        if (!allow_nesting) {
            klispE_throw_simple_with_irritants(K, "offset specifications cannot be nested", 1, v);
            return NULL;
        }
        TValue base_tv = kcar(v);
        TValue offset_tv = kcadr(v);
        if (!ttisfixint(offset_tv) || ivalue(offset_tv) < 0) {
            klispE_throw_simple_with_irritants(K, "offset should be nonnegative fixint", 1, v);
            return NULL;
        } else {
            size_t offset = ivalue(offset_tv);
            uint8_t * p = ffi_memory_location(K, false, base_tv, mutable, size + offset);
            return (p + offset);
        }
    } else {
        klispE_throw_simple_with_irritants(K, "not a memory location", 1, v);
        return NULL;
    }
}

void ffi_memmove(klisp_State *K, TValue *xparams,
                TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    bind_3tp(K, ptree,
            "any", anytype, dst_tv,
            "any", anytype, src_tv,
            "integer", ttisfixint, sz_tv);

    if (ivalue(sz_tv) < 0)
      klispE_throw_simple(K, "size should be nonnegative fixint");

    size_t sz = (size_t) ivalue(sz_tv);
    uint8_t * dst = ffi_memory_location(K, true, dst_tv, true, sz);
    const uint8_t * src = ffi_memory_location(K, true, src_tv, false, sz);
    memmove(dst, src, sz);

    kapply_cc(K, KINERT);
}

static void ffi_type_ref(klisp_State *K, TValue *xparams,
                         TValue ptree, TValue denv)
{
    UNUSED(denv);
    /*
    ** xparams[0]: pointer to ffi_codec_t
    */

    bind_1tp(K, ptree, "any", anytype, location_tv);
    ffi_codec_t *codec = pvalue(xparams[0]);
    const uint8_t *ptr = ffi_memory_location(K, true, location_tv, false, codec->libffi_type->size);
#if KGFFI_CHECK_ALIGNMENT
    if ((size_t) ptr % codec->libffi_type->alignment != 0)
      klispE_throw_simple(K, "unaligned memory read through FFI");
#endif

    TValue result = codec->decode(codec, K, ptr);
    kapply_cc(K, result);
}

static void ffi_type_set(klisp_State *K, TValue *xparams,
                         TValue ptree, TValue denv)
{
    UNUSED(denv);
    /*
    ** xparams[0]: pointer to ffi_codec_t
    */

    bind_2tp(K, ptree,
             "any", anytype, location_tv,
             "any", anytype, value_tv);
    ffi_codec_t *codec = pvalue(xparams[0]);
    uint8_t *ptr = ffi_memory_location(K, true, location_tv, false, codec->libffi_type->size);
#if KGFFI_CHECK_ALIGNMENT
    if ((size_t) ptr % codec->libffi_type->alignment != 0)
      klispE_throw_simple(K, "unaligned memory write through FFI");
#endif

    codec->encode(codec, K, value_tv, ptr);
    kapply_cc(K, KINERT);
}

void ffi_type_suite(klisp_State *K, TValue *xparams,
                    TValue ptree, TValue denv)
{
    bind_1tp(K, ptree, "string", ttisstring, type_tv);
    ffi_codec_t *codec = tv2ffi_codec(K, type_tv);

    TValue size_tv = i2tv(codec->libffi_type->size);
    krooted_tvs_push(K, size_tv);

    TValue alignment_tv = i2tv(codec->libffi_type->alignment);
    krooted_tvs_push(K, alignment_tv);

    TValue getter_tv =
        (codec->decode)
        ? kmake_applicative(K, ffi_type_ref, 1, p2tv(codec))
        : KINERT;
    krooted_tvs_push(K, getter_tv);

    TValue setter_tv =
        (codec->encode)
        ? kmake_applicative(K, ffi_type_set, 1, p2tv(codec))
        : KINERT;
    krooted_tvs_push(K, setter_tv);

    TValue suite_tv = kimm_list(K, 4, size_tv, alignment_tv, getter_tv, setter_tv);

    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    kapply_cc(K, suite_tv);
}

/* init ground */
void kinit_ffi_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /* create encapsulation keys */

    TValue dll_key = kmake_encapsulation_key(K);
    TValue cif_key = kmake_encapsulation_key(K);

    add_applicative(K, ground_env, "ffi-load-library", ffi_load_library, 1, dll_key);
    add_applicative(K, ground_env, "ffi-make-call-interface", ffi_make_call_interface, 1, cif_key);
    add_applicative(K, ground_env, "ffi-make-applicative", ffi_make_applicative, 2, dll_key, cif_key);
    add_applicative(K, ground_env, "ffi-memmove", ffi_memmove, 0);
    add_applicative(K, ground_env, "ffi-type-suite", ffi_type_suite, 0);
    add_applicative(K, ground_env, "ffi-library?", enc_typep, 1, dll_key);
    add_applicative(K, ground_env, "ffi-call-interface?", enc_typep, 1, cif_key);
}
