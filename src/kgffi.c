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

#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kerror.h"
#include "kblob.h"
#include "kencapsulation.h"

#include "kghelpers.h"
#include "kgencapsulations.h"
#include "kgffi.h"

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

static void ffi_encode_pointer(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    if (ttisblob(v)) {
        *(void **)buf = tv2blob(v)->b;
    } else if (ttisstring(v)) {
        *(void **)buf = kstring_buf(v);
    } else if (ttisnil(v)) {
        *(void **)buf = NULL;
    } else {
        klispE_throw_simple_with_irritants(K, "neither blob, string or nil", 1, v);
    }
}

static TValue ffi_decode_string(ffi_codec_t *self, klisp_State *K, const void *buf)
{
    UNUSED(self);
    return kstring_new_b_imm(K, *(char **)buf);
}

static void ffi_encode_string(ffi_codec_t *self, klisp_State *K, TValue v, void *buf)
{
    if (ttisstring(v)) {
        *(void **)buf = kstring_buf(v);
    } else {
        klispE_throw_simple_with_irritants(K, "not a string", 1, v);
    }
}

static ffi_codec_t ffi_codecs[] = {
    { "void", &ffi_type_void, ffi_decode_void, NULL },
    { "pointer", &ffi_type_pointer, NULL, ffi_encode_pointer },
    { "string", &ffi_type_pointer, ffi_decode_string, ffi_encode_string },
#define SIMPLE_TYPE(t) { #t, &ffi_type_ ## t, ffi_decode_ ## t, ffi_encode_ ## t }
    SIMPLE_TYPE(sint)
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
    add_applicative(K, ground_env, "ffi-library?", enc_typep, 1, dll_key);
    add_applicative(K, ground_env, "ffi-call-interface?", enc_typep, 1, cif_key);
}
