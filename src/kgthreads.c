/*
** kgstrings.c
** Strings features for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "ktable.h"
#include "kobject.h"
#include "kmutex.h"
#include "kcondvar.h"
#include "kghelpers.h"

/* ?.1? thread? */
/* uses typep */

/* ?.2? get-current-thread */
static void get_current_thread(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);
    check_0p(K, ptree);
    kapply_cc(K, gc2th(K));
}

static void *thread_run(void *data)
{
    klisp_State *K = (klisp_State *) data;

/* XXX/REFACTOR This is more or less the same that is repeated
 over and over again in the repl code (klisp.c), move to a helper
routine somewhere */
    bool errorp = false; /* may be set to true in error handler */
    bool rootp = true; /* may be set to false in continuation */

    /* ???/TODO should the fact that the thread thrown an exception
       be reported to the error output??? */
    
    /* We have already the appropriate environment,
       operative and arguments in place, but we still need the 
       continuations/guards */
    /* LOCK: We need the GIL for allocating the objects */
    klisp_lock(K);

    K->status = KLISP_THREAD_RUNNING;
    /* create the guard set error flag after errors */
    TValue exit_int = kmake_operative(K, do_int_mark_error, 
                                      1, p2tv(&errorp));
    krooted_tvs_push(K, exit_int);
    TValue exit_guard = kcons(K, G(K)->error_cont, exit_int);
    krooted_tvs_pop(K); /* already in guard */
    krooted_tvs_push(K, exit_guard);
    TValue exit_guards = kcons(K, exit_guard, KNIL);
    krooted_tvs_pop(K); /* already in guards */
    krooted_tvs_push(K, exit_guards);

    TValue entry_guards = KNIL;

    /* this is needed for interception code */
    TValue env = kmake_empty_environment(K);
    krooted_tvs_push(K, env);
    TValue outer_cont = kmake_continuation(K, G(K)->root_cont, 
                                           do_pass_value, 2, entry_guards, env);
    kset_outer_cont(outer_cont);
    krooted_tvs_push(K, outer_cont);
    TValue inner_cont = kmake_continuation(K, outer_cont, 
                                           do_pass_value, 2, exit_guards, env);
    kset_inner_cont(inner_cont);
    krooted_tvs_pop(K); krooted_tvs_pop(K); krooted_tvs_pop(K);

    krooted_tvs_push(K, inner_cont);

    /* This continuation will discard the result of the evaluation
       and return #inert instead, it will also signal via rootp = false
       that the evaluation didn't explicitly invoke the root continuation
    */
    TValue discard_cont = kmake_continuation(K, inner_cont, do_int_mark_root,
                                             1, p2tv(&rootp));

    krooted_tvs_pop(K); /* pop inner cont */
    krooted_tvs_push(K, discard_cont);

    kset_cc(K, discard_cont);
    krooted_tvs_pop(K); /* pop discard cont */

    klisp_unlock(K);

    /* LOCK: run will acquire the lock, and release it when done */
    klispT_run(K);

    klisp_lock(K);

    /* thread is done, we can remove it from the thread table */
    /* XXX what happens if this threads terminates abnormally?? */
    TValue *node = klispH_set(K, tv2table(G(K)->thread_table),
                              gc2th(K));
    *node = KFREE;

    K->status = errorp? KLISP_THREAD_ERROR : KLISP_THREAD_DONE;
    /* the thrown object/return value remains in K->next_obj */
    /* NOTICE that unless root continuation is explicitly invoked
       the value returned by the function is discarded!!
       This may change in the future */

    /* signal all threads waiting to join */
    int32_t ret = pthread_cond_broadcast(&K->joincond);
    klisp_assert(ret == 0); /* shouldn't happen */
    klisp_unlock(K);
    return NULL;
}

/* ?.3? make-thread */
static void make_thread(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "combiner", ttiscombiner, comb);
    TValue top = comb;
    while(ttisapplicative(top)) 
        top = kunwrap(top);

    /* GC: threads are fixed, no need to protect it */
    klisp_State *new_K = klispT_newthread(K);
    TValue new_th = gc2th(new_K);
    /* Prepare the new_K state to call the passed combiner with
       no arguments and an empty environment */
    /* TODO set_cc */
    klispT_set_cc(new_K, G(K)->root_cont);
    /* This will protect it from GC */
    new_K->next_env = kmake_empty_environment(K);
    TValue si = ktry_get_si(new_K, top);
    klispT_tail_call_si(new_K, top, KNIL, new_K->next_env, si);

    pthread_attr_t attr;
    int32_t ret = pthread_attr_init(&attr);
    klisp_assert(ret == 0); /* this shouldn't really happen... */
    /* make threads detached, the running state and return value
       will be kept in the corresponding klisp_State struct */
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    klisp_assert(ret == 0); /* this shouldn't really happen... */

    K->status = KLISP_THREAD_STARTING;
    ret = pthread_create(&new_K->thread, &attr, thread_run, new_K);

    if (ret != 0) {
        /* let the GC collect the failed State */
        resetbit(new_K->gct, FIXEDBIT);
        klispE_throw_simple_with_irritants(K, "Error creating thread", 
                                           1, i2tv(ret));
        return;
    }

    /* this shouldn't fail */
    UNUSED(pthread_attr_destroy(&attr));

    /* thread created correctly, return it */
    kapply_cc(K, new_th);
}

static void thread_join(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "thread", ttisthread, thread);

    if (tv_equal(gc2th(K), thread)) {
        klispE_throw_simple(K, "Thread can't join with itself");
        return;
    } else if (tv_equal(gc2th(G(K)->mainthread), thread)) {
        klispE_throw_simple(K, "Can't join with main thread");
        return;
    }

    klisp_State *K2 = tv2th(thread);
    
    while(true) {
        fflush(stdout);
        if (K2->status == KLISP_THREAD_DONE) {
            /* NOTICE that unless root continuation was explicitly invoked
               the value returned by the thread is discarded!!
               This may change in the future */
            kapply_cc(K, K2->next_value);
        } else if (K2->status == KLISP_THREAD_ERROR) {
            /* throw the same object, but in this thread */
            kcall_cont(K, G(K)->error_cont, K2->next_value);
            return;
        } else {
            /* must wait for this thread to end */
            /* LOCK: the GIL should be acquired exactly once */
            int32_t ret = pthread_cond_wait(&K2->joincond, &G(K)->gil);
            klisp_assert(ret == 0); /* shouldn't happen */
        }
    }
}

/* make-mutex */
static void make_mutex(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    check_0p(K, ptree);

    TValue new_mutex = kmake_mutex(K);
    kapply_cc(K, new_mutex);
}

/* mutex-lock */
static void mutex_lock(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "mutex", ttismutex, mutex);
    kmutex_lock(K, mutex);
    kapply_cc(K, KINERT);
}

/* mutex-unlock */
static void mutex_unlock(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "mutex", ttismutex, mutex);
    kmutex_unlock(K, mutex);
    kapply_cc(K, KINERT);
}

/* mutex-trylock */
static void mutex_trylock(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "mutex", ttismutex, mutex);
    bool res = kmutex_trylock(K, mutex);
    kapply_cc(K, b2tv(res));
}

/* make-mutex */
static void make_condvar(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "mutex", ttismutex, mutex);

    TValue new_condvar = kmake_condvar(K, mutex);
    kapply_cc(K, new_condvar);
}

/* condition-variable-wait */
static void condvar_wait(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(xparams);
    UNUSED(denv);

    bind_1tp(K, ptree, "condition-variable", ttiscondvar, condvar);
    kcondvar_wait(K, condvar);
    kapply_cc(K, KINERT);
}

/* condition-variable-signal / condition-variable-broadcast */
static void condvar_signal(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    /*
    ** xparams[0]: broadcast?
    */
    bool broadcast = bvalue(xparams[0]);

    bind_1tp(K, ptree, "condition-variable", ttiscondvar, condvar);
    kcondvar_signal(K, condvar, broadcast);
    kapply_cc(K, KINERT);
}

/* init ground */
void kinit_threads_ground_env(klisp_State *K)
{
    TValue ground_env = G(K)->ground_env;
    TValue symbol, value;

    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from a mix of scheme implementations and the pthreads library
    */

    /* ?.1? thread? */
    add_applicative(K, ground_env, "thread?", typep, 2, symbol, 
                    i2tv(K_TTHREAD));

    /* ?.2? get-current-thread */
    add_applicative(K, ground_env, "get-current-thread", get_current_thread, 0);

    /* ?.3? make-thread */
    add_applicative(K, ground_env, "make-thread", make_thread, 0);

    /* ?.4? thread-join */
    add_applicative(K, ground_env, "thread-join", thread_join, 0);

    /* Mutexes */
    /* mutex? */
    add_applicative(K, ground_env, "mutex?", typep, 2, symbol, 
                    i2tv(K_TMUTEX));

    /* make-mutex */
    add_applicative(K, ground_env, "make-mutex", make_mutex, 0);
    /* REFACTOR: should lock and unlock have an '!'?
       What about try lock?? '!', '?', '!?', neither? */
    /* mutex-lock */
    add_applicative(K, ground_env, "mutex-lock", mutex_lock, 0);
    /* mutex-unlock */
    add_applicative(K, ground_env, "mutex-unlock", mutex_unlock, 0);
    /* mutex-trylock */
    add_applicative(K, ground_env, "mutex-trylock", mutex_trylock, 0);

    /* Condition variables */
    /* condition-variable? */
    add_applicative(K, ground_env, "condition-variable?", typep, 2, symbol, 
                    i2tv(K_TCONDVAR));

    /* make-condition-variable */
    add_applicative(K, ground_env, "make-condition-variable", 
                    make_condvar, 0);
    /* REFACTOR: should signal have an '!'? */
    /* condition-variable-wait */
    add_applicative(K, ground_env, "condition-variable-wait", 
                    condvar_wait, 0);
    /* condition-variable-signal */
    add_applicative(K, ground_env, "condition-variable-signal", 
                    condvar_signal, 1, b2tv(false));
    /* condition-variable-broadcast */
    add_applicative(K, ground_env, "condition-variable-broadcast", 
                    condvar_signal, 1, b2tv(true));
}
