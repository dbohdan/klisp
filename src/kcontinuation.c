/*
** kcontinuation.c
** Kernel Continuations
** See Copyright Notice in klisp.h
*/

#include <stdarg.h>

#include "kcontinuation.h"
#include "kpair.h"
#include "kapplicative.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"

TValue kmake_continuation(klisp_State *K, TValue parent, klisp_CFunction fn, 
                          int32_t xcount, ...)
{
    va_list argp;

    Continuation *new_cont = (Continuation *)
        klispM_malloc(K, sizeof(Continuation) + sizeof(TValue) * xcount);

    /* header + gc_fields */
    klispC_link(K, (GCObject *) new_cont, K_TCONTINUATION, 
                K_FLAG_CAN_HAVE_NAME);


    /* continuation specific fields */
    new_cont->mark = KFALSE;    
    new_cont->parent = parent;

    TValue comb = K->next_obj;
    if (ttiscontinuation(comb))
        comb = tv2cont(comb)->comb;
    new_cont->comb = comb;

    new_cont->fn = fn;
    new_cont->extra_size = xcount;

    va_start(argp, xcount);
    for (int i = 0; i < xcount; i++) {
        new_cont->extra[i] = va_arg(argp, TValue);
    }
    va_end(argp);

    TValue res = gc2cont(new_cont);
    /* Add the current source info as source info (may be changed later) */
    /* TODO: find all the places where this should be changed (like $and?, 
       $sequence), and change it */
    kset_source_info(K, res, kget_csi(K));
    return res;
}

/*
**
** Interception Handling
**
*/

/* Helper for continuation->applicative */
/* this passes the operand tree to the continuation */
void cont_app(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue ptree = K->next_value;
    TValue denv = K->next_env;
    klisp_assert(ttisenvironment(K->next_env));
    UNUSED(denv);
    TValue cont = xparams[0];
    /* guards and dynamic variables are handled in kcall_cont() */
    kcall_cont(K, cont, ptree);
}

/* 
** This is used to determine if cont is in the dynamic extent of
** some other continuation. That's the case iff that continuation
** was marked by the call to mark_iancestors(cont) 
*/

/* TODO: maybe add some inlines here, profile first and check size difference */
/* LOCK: GIL should be acquired */
static void mark_iancestors(TValue cont) 
{
    while(!ttisnil(cont)) {
        kmark(cont);
        cont = tv2cont(cont)->parent;
    }
}

/* LOCK: GIL should be acquired */
static void unmark_iancestors(TValue cont) 
{
    while(!ttisnil(cont)) {
        kunmark(cont);
        cont = tv2cont(cont)->parent;
    }
}

/* 
** Returns the first interceptor whose dynamic extent includes cont
** or nil if there isn't any. The cont is implicitly passed because
** all of its improper ancestors are marked.
*/   
/* LOCK: GIL should be acquired */
static TValue select_interceptor(TValue guard_ls)
{
    /* the guard list can't be cyclic, that case is 
       replaced by a simple list while copyng guards */
    while(!ttisnil(guard_ls)) {
        /* entry is (selector . interceptor-op) */
        TValue entry = kcar(guard_ls);
        TValue selector = kcar(entry);
        if (kis_marked(selector))
            return kcdr(entry); /* only interceptor is important */
        guard_ls = kcdr(guard_ls);
    }
    return KNIL;
}

/* 
** Returns a list of entries like the following:
** (interceptor-op outer_cont . denv)
*/

/* GC: assume src_cont & dst_cont are rooted */
TValue create_interception_list(klisp_State *K, TValue src_cont, 
                                       TValue dst_cont)
{
    mark_iancestors(dst_cont);
    TValue ilist = kcons(K, KNIL, KNIL);
    krooted_vars_push(K, &ilist);
    TValue tail = ilist;
    TValue cont = src_cont;

    /* exit guards are from the inside to the outside, and
       selected by destination */

    /* the loop is until we find the common ancestor, that has to be marked */
    while(!kis_marked(cont)) {
        /* only inner conts have exit guards */
        if (kis_inner_cont(cont)) {
            klisp_assert(tv2cont(cont)->extra_size > 1);
            TValue entries = tv2cont(cont)->extra[0]; /* TODO make a macro */ 

            TValue interceptor = select_interceptor(entries);
            if (!ttisnil(interceptor)) {
                /* TODO make macros */
                TValue denv = tv2cont(cont)->extra[1]; 
                TValue outer = tv2cont(cont)->parent;
                TValue outer_denv = kcons(K, outer, denv);
                krooted_tvs_push(K, outer_denv);
                TValue new_entry = kcons(K, interceptor, outer_denv);
                krooted_tvs_pop(K); /* already in entry */
                krooted_tvs_push(K, new_entry);
                TValue new_pair = kcons(K, new_entry, KNIL);
                krooted_tvs_pop(K);
                kset_cdr(tail, new_pair);
                tail = new_pair;
            }
        }
        cont = tv2cont(cont)->parent;
    }
    unmark_iancestors(dst_cont);

    /* entry guards are from the outside to the inside, and
       selected by source, we create the list from the outside
       by cons and then append it to the exit list to avoid
       reversing */
    mark_iancestors(src_cont);

    cont = dst_cont;
    TValue entry_int = KNIL;
    krooted_vars_push(K, &entry_int);

    while(!kis_marked(cont)) {
        /* only outer conts have entry guards */
        if (kis_outer_cont(cont)) {
            klisp_assert(tv2cont(cont)->extra_size > 1);
            TValue entries = tv2cont(cont)->extra[0]; /* TODO make a macro */
            /* this is rooted because it's a substructure of entries */
            TValue interceptor = select_interceptor(entries);
            if (!ttisnil(interceptor)) {
                /* TODO make macros */
                TValue denv = tv2cont(cont)->extra[1]; 
                TValue outer = cont;
                TValue outer_denv = kcons(K, outer, denv);
                krooted_tvs_push(K, outer_denv);
                TValue new_entry = kcons(K, interceptor, outer_denv);
                krooted_tvs_pop(K); /* already in entry */
                krooted_tvs_push(K, new_entry);
                entry_int = kcons(K, new_entry, entry_int);
                krooted_tvs_pop(K);
            }
        }
        cont = tv2cont(cont)->parent;
    }

    unmark_iancestors(src_cont);
    
    /* all interceptions collected, append the two lists and return */
    kset_cdr(tail, entry_int);

    krooted_vars_pop(K);
    krooted_vars_pop(K);
    return kcdr(ilist);
}

void do_interception(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    /* 
    ** xparams[0]: 
    ** xparams[1]: dst cont
    */
    TValue ls = xparams[0];
    TValue dst_cont = xparams[1];
    if (ttisnil(ls)) {
        /* all interceptors returned normally */
        /* this is a normal pass/not subject to interception */
        kset_cc(K, dst_cont);
        kapply_cc(K, obj);
    } else {
        /* call the operative with the passed obj and applicative
           for outer cont as ptree in the dynamic environment of 
           the corresponding call to guard-continuation in the 
           dynamic extent of the associated outer continuation.
           If the operative normally returns a value, others
           interceptions should be scheduled */
        TValue first = kcar(ls);
        TValue op = kcar(first);
        TValue outer = kcadr(first);
        TValue denv = kcddr(first);
        TValue app = kmake_applicative(K, cont_app, 1, outer);
        krooted_tvs_push(K, app);
        TValue ptree = klist(K, 2, obj, app);
        krooted_tvs_pop(K); /* already in ptree */
        krooted_tvs_push(K, ptree);
        TValue new_cont = kmake_continuation(K, outer, do_interception,
                                             2, kcdr(ls), dst_cont);
        kset_cc(K, new_cont);
        krooted_tvs_pop(K);
        /* XXX: what to pass as si? */
        ktail_call(K, op, ptree, denv);
    }
}
