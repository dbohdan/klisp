
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "klisp.h"
#include "kpair.h"
#include "kstate.h"
#include "kmem.h"
#include "kstring.h"
#include "kerror.h"

/* TODO: check that all objects passed to throw are rooted */

/* GC: assumes all objs passed are rooted */
TValue klispE_new(klisp_State *K, TValue who, TValue cont, TValue msg, 
                  TValue irritants) 
{
    klisp_lock(K);
    Error *new_error = klispM_new(K, Error);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_error, K_TERROR, 0);

    /* error specific fields */
    new_error->who = who;
    new_error->cont = cont;
    new_error->msg = msg;
    new_error->irritants = irritants;
    klisp_unlock(K);
    return gc2error(new_error);
}

TValue klispE_new_with_errno_irritants(klisp_State *K, const char *service, 
                                       int errnum, TValue irritants)
{
    TValue error_description = klispE_describe_errno(K, service, errnum);
    krooted_tvs_push(K, error_description);
    TValue all_irritants = kimm_cons(K, error_description, irritants);
    krooted_tvs_push(K, all_irritants);
    TValue error_obj = klispE_new(K, K->next_obj, K->curr_cont,
                                  kcaddr(error_description),
                                  all_irritants);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    return error_obj;
}

/* This is meant to be called by the GC */
/* LOCK: GIL should be acquired */
void klispE_free(klisp_State *K, Error *error)
{
    klispM_free(K, error);
}

/*
** Clear all stacks & buffers 
*/
void clear_buffers(klisp_State *K)
{
    /* These shouldn't cause GC, but just in case do them first,
       an object may be protected in tvs or vars */
    ks_sclear(K);
    ks_tbclear(K);
    K->shared_dict = KNIL;

    krooted_tvs_clear(K);
    krooted_vars_clear(K);
}

/*
** Throw a simple error obj with:
** {
**        who: current operative/continuation, 
**        cont: current continuation, 
**        message: msg, 
**        irritants: ()
** }
*/
/* GC: assumes all objs passed are rooted */
void klispE_throw_simple(klisp_State *K, char *msg)
{
    TValue error_msg = kstring_new_b_imm(K, msg);
    krooted_tvs_push(K, error_msg);
    TValue error_obj = 
        klispE_new(K, K->next_obj, K->curr_cont, error_msg, KNIL);
    /* clear buffer shouldn't cause GC, but just in case... */
    krooted_tvs_push(K, error_obj);
    clear_buffers(K); /* this pops both error_msg & error_obj */
    klisp_unlock_all(K); /* is this thread holds the GIL release it */
    /* call_cont protects error from gc */
    kcall_cont(K, G(K)->error_cont, error_obj);
}

/*
** Throw an error obj with:
** {
**        who: current operative/continuation, 
**        cont: current continuation, 
**        message: msg, 
**        irritants: irritants
** }
*/
/* GC: assumes all objs passed are rooted */
void klispE_throw_with_irritants(klisp_State *K, char *msg, TValue irritants)
{
    /* it's important that this is immutable, because it's user
       accessible */
    TValue error_msg = kstring_new_b_imm(K, msg);
    krooted_tvs_push(K, error_msg);
    TValue error_obj = 
        klispE_new(K, K->next_obj, K->curr_cont, error_msg, irritants);
    /* clear buffer shouldn't cause GC, but just in case... */
    krooted_tvs_push(K, error_obj);
    clear_buffers(K); /* this pops both error_msg & error_obj */
    klisp_unlock_all(K); /* is this thread holds the GIL release it */
    /* call_cont protects error from gc */
    kcall_cont(K, G(K)->error_cont, error_obj);
}

void klispE_throw_system_error_with_irritants(
    klisp_State *K, const char *service, int errnum, TValue irritants)
{
    TValue error_obj = klispE_new_with_errno_irritants(K, service, errnum, 
                                                       irritants);
    krooted_tvs_push(K, error_obj);
    clear_buffers(K);
    klisp_unlock_all(K); /* is this thread holds the GIL release it */
    kcall_cont(K, G(K)->system_error_cont, error_obj);
}

/* The array symbolic_error_codes[] assigns locale and target
 * platform configuration independent strings to errno values.
 *
 * Generated from Linux header files:
 *
 * awk '{printf("    c(%s),\n", $2)}' /usr/include/asm-generic/errno-base.h
 * awk '{printf("    c(%s),\n", $2)}' /usr/include/asm-generic/errno.h
 *
 * and removed errnos not present in mingw.
 *
 */
#define c(N) [N] = # N
static const char * const symbolic_error_codes[] = {
    c(EPERM),
    c(ENOENT),
    c(ESRCH),
    c(EINTR),
    c(EIO),
    c(ENXIO),
    c(E2BIG),
    c(ENOEXEC),
    c(EBADF),
    c(ECHILD),
    c(EAGAIN),
    c(ENOMEM),
    c(EACCES),
    c(EFAULT),
    c(EBUSY),
    c(EEXIST),
    c(EXDEV),
    c(ENODEV),
    c(ENOTDIR),
    c(EISDIR),
    c(EINVAL),
    c(ENFILE),
    c(EMFILE),
    c(ENOTTY),
    c(EFBIG),
    c(ENOSPC),
    c(ESPIPE),
    c(EROFS),
    c(EMLINK),
    c(EPIPE),
    c(EDOM),
    c(ERANGE),
    /**/
    c(EDEADLK),
    c(ENAMETOOLONG),
    c(ENOLCK),
    c(ENOSYS),
    c(ENOTEMPTY),
};
#undef c

/* klispE_describe_errno(K, ERRNUM, SERVICE) returns a list
 *
 *    (SERVICE CODE MESSAGE ERRNUM)
 *
 *  SERVICE (string) identifies the failed system call or service,
 *  e.g. "rename" or "fopen".
 *
 *  CODE (string) is a platform-independent symbolic representation
 *  of the error. It corresponds to symbolic constants of <errno.h>,
 *  e.g. "ENOENT" or "ENOMEM".
 *
 *  MESSAGE (string) platform-dependent human-readable description.
 *  The MESSAGE may depend on locale or operating system configuration.
 *
 *  ERRNUM (fixint) is the value of errno for debugging puroposes.
 *
 */
TValue klispE_describe_errno(klisp_State *K, const char *service, int errnum)
{
    const char *code = NULL;
    int tabsize = sizeof(symbolic_error_codes) / 
        sizeof(symbolic_error_codes[0]);
    if (0 <= errnum && errnum < tabsize)
        code = symbolic_error_codes[errnum];
    if (code == NULL)
        code = "UNKNOWN";

    TValue service_tv = kstring_new_b_imm(K, service);
    krooted_tvs_push(K, service_tv);
    TValue code_tv = kstring_new_b_imm(K, code);
    krooted_tvs_push(K, code_tv);
    TValue message_tv = kstring_new_b_imm(K, strerror(errnum));
    krooted_tvs_push(K, message_tv);

    TValue v = kimm_list(K, 4, service_tv, code_tv, message_tv, i2tv(errnum));
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
    return v;
}
