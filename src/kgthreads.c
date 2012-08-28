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
#include "kobject.h"

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
    
    /* We have already the appropriate environment,
       operative and arguments in place, but we still need the 
       continuations/guards */
    /* LOCK: We need the GIL for allocating the objects */
    klisp_lock(K);
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

    /* LOCK: run will acquire the lock */
    klispT_run(K);

    /* TODO get the return value */
/*    
    int status = errorp? STATUS_ERROR : 
        (rootp? STATUS_ROOT : STATUS_CONTINUE);
*/

/* /XXX     */


    /* thread is done, we can remove the fixed bit */
    /* XXX what happens if this threads terminates abnormally?? */
    klisp_lock(K);
    resetbit(K->gct, FIXEDBIT);
    klisp_unlock(K);
    /* TODO return value */
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
    int32_t ret = pthread_create(&new_K->thread, NULL, thread_run, new_K);

    if (ret != 0) {
        /* let the GC collect the failed State */
        resetbit(new_K->gct, FIXEDBIT);
        klispE_throw_simple_with_irritants(K, "Error creating thread", 
                                           1, i2tv(ret));
        return;
    }

    /* thread created correctly, return it */
    kapply_cc(K, new_th);
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
}
