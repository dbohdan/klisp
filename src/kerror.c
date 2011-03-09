
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "klisp.h"
#include "kstate.h"
#include "kmem.h"
#include "kstring.h"

void clear_buffers(klisp_State *K)
{
    /* XXX: clear stack and char buffer, clear shared dict */
    /* TODO: put these in handlers for read-token, read and write */
    ks_sclear(K);
    ks_tbclear(K);
    K->shared_dict = KNIL;
}

void klispE_throw(klisp_State *K, char *msg)
{
    TValue error_msg = kstring_new(K, msg, strlen(msg));
    /* TEMP */
    clear_buffers(K);

    kcall_cont(K, K->error_cont, error_msg);
}

/* TEMP: for throwing with extra msg info */
void klispE_throw_extra(klisp_State *K, char *msg, char *extra_msg) {
    /* TODO */
    int32_t l1 = strlen(msg);
    int32_t l2 = strlen(extra_msg);

    int32_t tl = l1+l2+1;

    char *msg_buf = klispM_malloc(K, tl);
    strcpy(msg_buf, msg);
    strcpy(msg_buf+l1, extra_msg);

    TValue error_msg = kstring_new(K, msg_buf, tl);
    klispM_freemem(K, msg_buf, tl+1);

    clear_buffers(K);

    kcall_cont(K, K->error_cont, error_msg);
}
