/*
** ffi-signal.c
**
** Example of interpreter extension. Please follow instructions
** in ffi-signal.k.
**
*/

#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "kstate.h"
#include "kstring.h"
#include "kport.h"
#include "kghelpers.h"

#if !defined(KLISP_USE_POSIX) || !defined(KUSE_LIBFFI)
#  error "Bad klisp configuration."
#endif

static int self_pipe[2];

static void handler(int signo)
{
    uint8_t message = (uint8_t) signo;
    write(self_pipe[1], &message, 1);
}

static void install_signal_handler(klisp_State *K, TValue *xparams,
                                   TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);
    bind_1tp(K, ptree, "string", ttisstring, signame);
    int signo;

    if (!strcmp(kstring_buf(signame), "SIGINT")) {
        signo = SIGINT;
    } else if (!strcmp(kstring_buf(signame), "SIGCLD")) {
        signo = SIGCLD;
    } else {
        klispE_throw_simple_with_irritants(K, "unsupported signal", 1, signame);
        return;
    }
    signal(signo, handler);
    kapply_cc(K, KINERT);
}

static void open_signal_port(klisp_State *K, TValue *xparams,
                             TValue ptree, TValue denv)
{
    UNUSED(xparams);
    UNUSED(denv);

    FILE *fw = fdopen(self_pipe[0], "r");
    TValue filename = kstring_new_b_imm(K, "**SIGNAL**");
    krooted_tvs_push(K, filename);
    TValue port = kmake_std_port(K, filename, false, fw);
    krooted_tvs_pop(K);
    kapply_cc(K, port);
}

static void safe_add_applicative(klisp_State *K, TValue env,
                                 const char *name,
                                 klisp_Ofunc fn)
{
    TValue symbol = ksymbol_new(K, name, KNIL);
    krooted_tvs_push(K, symbol);
    TValue value = kmake_applicative(K, fn, 0);
    krooted_tvs_push(K, value);
    kadd_binding(K, env, symbol, value);
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);
}

void kinit_signal_example(klisp_State *K)
{
    pipe(self_pipe);
    fcntl(self_pipe[1], F_SETFL, O_NONBLOCK);
    safe_add_applicative(K, K->next_env, "install-signal-handler", install_signal_handler);
    safe_add_applicative(K, K->next_env, "open-signal-port", open_signal_port);
    klisp_assert(K->rooted_tvs_top == 0);
    klisp_assert(K->rooted_vars_top == 0);
}
