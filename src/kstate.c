/*
** kstate.c
** klisp vm state
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is mostly from Lua.
** The algorithm for testing if a continuation is within the dynamic extent
** of another continuation using marks is by John Shutt. The implementation
** he uses (see SINK) is in scheme and is under the GPL but I think this is 
** different enough (and the algorithm simple/small enough) to avoid any 
** problem. ASK John.
*/

#include <stdlib.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <pthread.h>

#include "klisp.h"
#include "klimits.h"
#include "kstate.h"
#include "kobject.h"
#include "kpair.h"
#include "kmem.h"
#include "keval.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kcontinuation.h"
#include "kenvironment.h"
#include "kground.h"
#include "krepl.h"
#include "ksymbol.h"
#include "kstring.h"
#include "kport.h"
#include "ktable.h"
#include "kbytevector.h"
#include "kvector.h"

#include "kghelpers.h" /* for creating list_app & memoize_app */
#include "kgerrors.h" /* for creating error hierarchy */

#include "kgc.h" /* for memory freeing & gc init */


/* in lua state size can have an extra space here to save
   some user data, for now we don't have that in klisp */
#define state_size(x) (sizeof(x) + 0)
#define fromstate(k)	(cast(uint8_t *, (k)) - 0)
#define tostate(k)   (cast(klisp_State *, cast(uint8_t *, k) + 0))

/*
** Main thread combines a thread state and the global state
*/
typedef struct KG {
  klisp_State k;
  global_State g;
} KG;

/*
** open parts that may cause memory-allocation errors
*/
static void f_klispopen (klisp_State *K, void *ud) {
    global_State *g = G(K);
    UNUSED(ud);
    klispS_resize(K, MINSTRTABSIZE);  /* initial size of string table */

    void *s = (*g->frealloc)(ud, NULL, 0, KS_ISSIZE * sizeof(TValue));
    if (s == NULL) { 
        return; /* XXX throw error somehow & free mem */
    }
    void *b = (*g->frealloc)(ud, NULL, 0, KS_ITBSIZE);
    if (b == NULL) {
        return; /* XXX throw error somehow & free mem */
    }

    /* initialize temp stacks */
    ks_ssize(K) = KS_ISSIZE;
    ks_stop(K) = 0; /* stack is empty */
    ks_sbuf(K) = (TValue *)s;

    ks_tbsize(K) = KS_ITBSIZE;
    ks_tbidx(K) = 0; /* buffer is empty */
    ks_tbuf(K) = (char *)b;
    
    /* (at least for now) we'll use a non recursive mutex for the GIL */
    /* XXX/TODO check return code */
    pthread_mutex_init(&g->gil, NULL);

/* This is here in lua, but in klisp we still need to alloc
   a bunch of objects:
   g->GCthreshold = 4*g->totalbytes; 
*/
}


static void preinit_state (klisp_State *K, global_State *g) {
    G(K) = g;

    K->gil_count = 0;
    K->curr_cont = KNIL;
    K->next_obj = KINERT;
    K->next_func = NULL;
    K->next_value = KINERT;
    K->next_env = KNIL;
    K->next_xparams = NULL;
    K->next_si = KNIL;

    /* current input and output */
    K->curr_port = KINERT; /* set on each call to read/write */

    /* init the stacks used to protect variables & values from gc,
       this should be done before any new object is created because
       they are used by them */
    K->rooted_tvs_top = 0;
    K->rooted_vars_top = 0;

    /* initialize tokenizer */

    /* WORKAROUND: for stdin line buffering & reading of EOF */
    K->ktok_seen_eof = false;

    /* TEMP: For now just hardcode it to 8 spaces tab-stop */
    K->ktok_source_info.tab_width = 8;
    /* all three are set on each call to read */
    K->ktok_source_info.filename = KINERT; 
    K->ktok_source_info.line = 1; 
    K->ktok_source_info.col = 0;

    K->ktok_nested_comments = 0;

    /* initialize reader */
    K->shared_dict = KNIL;
    K->read_mconsp = false; /* set on each call to read */

    /* initialize writer */
    K->write_displayp = false; /* set on each call to write */

    /* put zeroes first, in case alloc fails */
    ks_stop(K) = 0;
    ks_ssize(K) = 0; 
    ks_sbuf(K) = NULL;

    ks_tbidx(K) = 0;
    ks_tbsize(K) = 0;
    ks_tbuf(K) = NULL;
}

static void close_state(klisp_State *K)
{
    global_State *g = G(K);

    /* collect all objects */
    klispC_freeall(K);
    klisp_assert(g->rootgc == obj2gco(K));
    klisp_assert(g->strt.nuse == 0);

    /* free helper buffers */
    klispM_freemem(K, ks_sbuf(K), ks_ssize(K) * sizeof(TValue));
    klispM_freemem(K, ks_tbuf(K), ks_tbsize(K));
    /* free string/symbol table */
    klispM_freearray(K, g->strt.hash, G(K)->strt.size, GCObject *);

    /* destroy the GIL */
    pthread_mutex_destroy(&g->gil);

    /* only remaining mem should be of the state struct */
    klisp_assert(g->totalbytes == sizeof(KG));
    /* NOTE: this needs to be done "by hand" */
    (*g->frealloc)(g->ud, fromstate(K), state_size(KG), 0);
}

/*
** State creation and destruction
*/
klisp_State *klisp_newstate(klisp_Alloc f, void *ud)
{
    klisp_State *K;
    global_State *g;
    
    void *k = (*f)(ud, NULL, 0, state_size(KG));
    if (k == NULL) return NULL;
    K = tostate(k);
    g = &((KG *)K)->g;
    /* Init klisp_State object header (for GC) */
    K->next = NULL;
    K->tt = K_TTHREAD;
    K->kflags = 0;
    K->si = NULL;
    g->currentwhite = bit2mask(WHITE0BIT, FIXEDBIT);
    K->gct = klispC_white(g);
    set2bits(K->gct, FIXEDBIT, SFIXEDBIT);

    preinit_state(K, g);

    ktok_init(K); /* initialize tokenizer tables */
    g->frealloc = f;
    g->ud = ud;
    g->mainthread = K;

    g->GCthreshold = 0;  /* mark it as unfinished state */

    /* these will be properly initialized later */
    g->strt.size = 0;
    g->strt.nuse = 0;
    g->strt.hash = NULL;
    g->name_table = KINERT;
    g->cont_name_table = KINERT;

    g->empty_string = KINERT;
    g->empty_bytevector = KINERT;
    g->empty_vector = KINERT;

    g->ktok_lparen = KINERT;
    g->ktok_rparen = KINERT;
    g->ktok_dot = KINERT;
    g->ktok_sexp_comment = KINERT;

    g->require_path = KINERT;
    g->require_table = KINERT;
    g->libraries_registry = KINERT;

    g->eval_op = KINERT;
    g->list_app = KINERT;
    g->memoize_app = KINERT;
    g->ground_env = KINERT;
    g->module_params_sym = KINERT;
    g->root_cont = KINERT;
    g->error_cont = KINERT;
    g->system_error_cont = KINERT;

    /* input / output for dynamic keys */
    /* these are init later */
    g->kd_in_port_key = KINERT;
    g->kd_out_port_key = KINERT;
    g->kd_error_port_key = KINERT;

    /* strict arithmetic dynamic key */
    /* this is init later */
    g->kd_strict_arith_key = KINERT;

    g->gcstate = GCSpause;
    g->rootgc = obj2gco(K); /* was NULL in unithread klisp... CHECK */
    g->sweepstrgc = 0;
    g->sweepgc = &g->rootgc;
    g->gray = NULL;
    g->grayagain = NULL;
    g->weak = NULL;
    g->tmudata = NULL;
    g->totalbytes = sizeof(KG);
    g->gcpause = KLISPI_GCPAUSE;
    g->gcstepmul = KLISPI_GCMUL;
    g->gcdept = 0;

    /* GC */
    g->totalbytes = state_size(KG) + KS_ISSIZE * sizeof(TValue) +
        KS_ITBSIZE;
    g->GCthreshold = UINT32_MAX; /* we still have a lot of allocation
                                    to do, put a very high value to 
                                    avoid collection */
    g->estimate = 0; /* doesn't matter, it is set by gc later */
    /* XXX Things start being ugly from here on...
       I have to think about the whole init procedure, for now
       I am mostly following lua, but the differences between it and 
       klisp show... We still have to allocate a lot of objects and 
       it isn't really clear what happens if we run out of space before
       all objects are allocated.  For now let's suppose that will not
       happen... */
    /* TODO handle errors, maybe with longjmp, also see lua
     luaD_rawrunprotected */
    f_klispopen(K, NULL); /* this touches GCthreshold */

    g->GCthreshold = UINT32_MAX; /* we still have a lot of allocation
                                    to do, put a very high value to 
                                    avoid collection */

    /* TEMP: err */
    /* THIS MAY CRASH THE INTERPRETER IF THERE IS AN ERROR IN THE INIT */
    /* do nothing for now */

    /* initialize strings */

    /* initialize name info table */
    /* needs weak keys, otherwise every named object would
       be fixed! */
    g->name_table = klispH_new(K, 0, MINNAMETABSIZE, 
                               K_FLAG_WEAK_KEYS);
    /* here the keys are uncollectable */
    g->cont_name_table = klispH_new(K, 0, MINCONTNAMETABSIZE, 
                                    K_FLAG_WEAK_NOTHING);

    /* Empty string */
    /* MAYBE: fix it so we can remove empty_string from roots */
    g->empty_string = kstring_new_b_imm(K, "");

    /* Empty bytevector */
    /* MAYBE: fix it so we can remove empty_bytevector from roots */
    /* XXX: find a better way to do this */
    g->empty_bytevector = KNIL; /* trick constructor to create empty bytevector */
    g->empty_bytevector = kbytevector_new_bs_imm(K, NULL, 0);

    /* Empty vector */
    /* MAYBE: see above */
    g->empty_vector = kvector_new_bs_g(K, false, NULL, 0);

    /* Special Tokens */
    g->ktok_lparen = kcons(K, ch2tv('('), KNIL);
    g->ktok_rparen = kcons(K, ch2tv(')'), KNIL);
    g->ktok_dot = kcons(K, ch2tv('.'), KNIL);
    g->ktok_sexp_comment = kcons(K, ch2tv(';'), KNIL);

    /* initialize require facilities */ 
    {
        char *str = getenv(KLISP_PATH);
        if (str == NULL)
            str = KLISP_PATH_DEFAULT;
	
        g->require_path = kstring_new_b_imm(K, str);
        /* replace dirsep with forward slashes,
           windows will happily accept forward slashes */
        str = kstring_buf(g->require_path);
        while ((str = strchr(str, *KLISP_DIRSEP)) != NULL)
            *str++ = '/';
    }
    g->require_table = klispH_new(K, 0, MINREQUIRETABSIZE, 0);

    /* initialize library facilities */
    g->libraries_registry = KNIL;

    /* the dynamic ports and the keys for the dynamic ports */
    TValue in_port = kmake_std_fport(K, kstring_new_b_imm(K, "*STDIN*"),
                                     false, false,  stdin);
    TValue out_port = kmake_std_fport(K, kstring_new_b_imm(K, "*STDOUT*"),
                                      true, false, stdout);
    TValue error_port = kmake_std_fport(K, kstring_new_b_imm(K, "*STDERR*"),
                                        true, false, stderr);
    g->kd_in_port_key = kcons(K, KTRUE, in_port);
    g->kd_out_port_key = kcons(K, KTRUE, out_port);
    g->kd_error_port_key = kcons(K, KTRUE, error_port);

    /* strict arithmetic key, (starts as false) */
    g->kd_strict_arith_key = kcons(K, KTRUE, KFALSE);

    /* create the ground environment and the eval operative */
    int32_t line_number; 
    TValue si;
    g->eval_op = kmake_operative(K, keval_ofn, 0), line_number = __LINE__;
#if KTRACK_SI
    si = kcons(K, kstring_new_b_imm(K, __FILE__), 
               kcons(K, i2tv(line_number), i2tv(0)));
    kset_source_info(K, g->eval_op, si);
#endif
    /* TODO: si */
    TValue eval_name = ksymbol_new_b(K, "eval", KNIL);
    ktry_set_name(K, g->eval_op, eval_name);
    
    g->list_app = kmake_applicative(K, list, 0), line_number = __LINE__;
#if KTRACK_SI
    si = kcons(K, kstring_new_b_imm(K, __FILE__), 
               kcons(K, i2tv(__LINE__), i2tv(0)));
    kset_source_info(K, g->list_app, si);
    kset_source_info(K, kunwrap(g->list_app), si);
#endif

    g->memoize_app = kmake_applicative(K, memoize, 0), line_number = __LINE__;
#if KTRACK_SI
    si = kcons(K, kstring_new_b_imm(K, __FILE__), 
               kcons(K, i2tv(__LINE__), i2tv(0)));
    kset_source_info(K, g->memoize_app, si);
    kset_source_info(K, kunwrap(g->memoize_app), si);
#endif
    /* ground environment has a hashtable for bindings */
    g->ground_env = kmake_table_environment(K, KNIL);
//    g->ground_env = kmake_empty_environment(K);

    /* MAYBE: fix it so we can remove module_params_sym from roots */
    /* TODO si */
    g->module_params_sym = ksymbol_new_b(K, "module-parameters", KNIL);

    /* Create the root and error continuation (will be added to the 
       environment in kinit_ground_env) */
    g->root_cont = kmake_continuation(K, KNIL, do_root_exit, 0);

#if KTRACK_SI
    /* Add source info to the cont */
    TValue str = kstring_new_b_imm(K, __FILE__);
    TValue tail = kcons(K, i2tv(__LINE__), i2tv(0));
    si = kcons(K, str, tail);
    kset_source_info(K, g->root_cont, si);
#endif

    g->error_cont = kmake_continuation(K, g->root_cont, do_error_exit, 0);

#if KTRACK_SI
    str = kstring_new_b_imm(K, __FILE__);
    tail = kcons(K, i2tv(__LINE__), i2tv(0));
    si = kcons(K, str, tail);
    kset_source_info(K, g->error_cont, si);
#endif

    /* this must be done before calling kinit_ground_env */
    kinit_error_hierarchy(K); 
    kinit_ground_env(K);
    kinit_cont_names(K);

    /* create a std environment and leave it in g->next_env */
    K->next_env = kmake_table_environment(K, g->ground_env);

    /* set the threshold for gc start now that we have allocated all mem */ 
    g->GCthreshold = 4*g->totalbytes;

    /* luai_userstateopen(L); */
    return K;
}

/* this is in api.c in lua */
klisp_State *klisp_newthread(klisp_State *K)
{
    /* TODO */
    return K;
}

/* XXX lock? */
klisp_State *klispT_newthread(klisp_State *K)
{
    klisp_State *K1 = tostate(klispM_malloc(K, state_size(klisp_State)));
    klispC_link(K, (GCObject *) K1, K_TTHREAD, 0);
    preinit_state(K1, G(K));

    /* initialize temp stacks */
    ks_sbuf(K1) = (TValue *) klispM_malloc(K, KS_ISSIZE * sizeof(TValue));
    ks_ssize(K1) = KS_ISSIZE;
    ks_stop(K1) = 0; /* stack is empty */

    ks_tbuf(K1) = (char *) klispM_malloc(K, KS_ITBSIZE);
    ks_tbsize(K1) = KS_ITBSIZE;
    ks_tbidx(K1) = 0; /* buffer is empty */
  
    klisp_assert(iswhite((GCObject *) (K1)));
    return K1;
}


/* XXX lock? */
void klispT_freethread (klisp_State *K, klisp_State *K1)
{
    klispM_freemem(K, ks_sbuf(K1), ks_ssize(K1) * sizeof(TValue));
    klispM_freemem(K, ks_tbuf(K1), ks_tbsize(K1));
    /* userstatefree() */
    klispM_freemem(K, fromstate(K1), state_size(klisp_State));
}

void klisp_close (klisp_State *K)
{
    K = G(K)->mainthread;  /* only the main thread can be closed */

/* XXX lua does the following */
#if 0 
    lua_lock(L); 
    luaF_close(L, L->stack);  /* close all upvalues for this thread */
    luaC_separateudata(L, 1);  /* separate udata that have GC metamethods */
    L->errfunc = 0;  /* no error function during GC metamethods */    /* free all collectable objects */
  do {  /* repeat until no more errors */
    L->ci = L->base_ci;
    L->base = L->top = L->ci->base;
    L->nCcalls = L->baseCcalls = 0;
  } while (luaD_rawrunprotected(L, callallgcTM, NULL) != 0);
  lua_assert(G(L)->tmudata == NULL);
  luai_userstateclose(L);
#endif

  /* luai_userstateclose(L); */
    close_state(K);
}

/*
** Root and Error continuations
*/
void do_root_exit(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    UNUSED(xparams);

    /* Just save the value and end the loop */
    K->next_value = obj;
    K->next_func = NULL;        /* force the loop to terminate */
    return;
}

void do_error_exit(klisp_State *K)
{
    TValue *xparams = K->next_xparams;
    TValue obj = K->next_value;
    klisp_assert(ttisnil(K->next_env));
    UNUSED(xparams);

    /* TEMP Just pass the error to the root continuation */
    kapply_cc(K, obj);
}

/*
** Stacks memory management
*/

/* TODO test this */
void ks_sgrow(klisp_State *K, int32_t new_top)
{
    size_t old_size = ks_ssize(K);
    /* should be powers of two multiple of KS_ISIZE */
    /* TEMP: do it naively for now */
    size_t new_size = old_size * 2;
    while(new_top > new_size)
        new_size *= 2;

    ks_sbuf(K) = klispM_realloc_(K, ks_sbuf(K), old_size*sizeof(TValue),
                                 new_size*sizeof(TValue));
    ks_ssize(K) = new_size; 
}

void ks_sshrink(klisp_State *K, int32_t new_top)
{
    /* NOTE: may shrink more than once, take it to a multiple of 
       KS_ISSIZE that is a power of 2 and no smaller than (size * 4) */
    size_t old_size = ks_ssize(K);
    /* TEMP: do it naively for now */
    size_t new_size = old_size;
    while(new_size > KS_ISSIZE && new_top * 4 < new_size)
        new_size /= 2;

    /* NOTE: shrink can't fail */
    ks_sbuf(K) = klispM_realloc_(K, ks_sbuf(K), old_size*sizeof(TValue),
                                 new_size*sizeof(TValue));
    ks_ssize(K) = new_size;
}


/* TODO test this */
void ks_tbgrow(klisp_State *K, int32_t new_top)
{
    size_t old_size = ks_tbsize(K);
    /* should be powers of two multiple of KS_ISIZE */
    /* TEMP: do it naively for now */
    size_t new_size = old_size * 2;
    while(new_top > new_size)
        new_size *= 2;

    ks_tbuf(K) = klispM_realloc_(K, ks_tbuf(K), old_size*sizeof(TValue),
                                 new_size*sizeof(TValue));
    ks_tbsize(K) = new_size; 
}

void ks_tbshrink(klisp_State *K, int32_t new_top)
{
    /* NOTE: may shrink more than once, take it to a multiple of 
       KS_ISSIZE that is a power of 2 and no smaller than (size * 4) */
    size_t old_size = ks_tbsize(K);
    /* TEMP: do it naively for now */
    size_t new_size = old_size;
    while(new_size > KS_ISSIZE && new_top * 4 < new_size)
        new_size /= 2;

    /* NOTE: shrink can't fail */
    ks_tbuf(K) = klispM_realloc_(K, ks_tbuf(K), old_size*sizeof(TValue),
                                 new_size*sizeof(TValue));
    ks_tbsize(K) = new_size;
}


/*
**
** This is for handling interceptions
** TODO: move to a different file
**
*/

/* 
** This is used to determine if cont is in the dynamic extent of
** some other continuation. That's the case iff that continuation
** was marked by the call to mark_iancestors(cont) 
*/

/* TODO: maybe add some inlines here, profile first and check size difference */
void mark_iancestors(TValue cont) 
{
    while(!ttisnil(cont)) {
        kmark(cont);
        cont = tv2cont(cont)->parent;
    }
}

void unmark_iancestors(TValue cont) 
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
TValue select_interceptor(TValue guard_ls)
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
static inline TValue create_interception_list(klisp_State *K, TValue src_cont, 
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

/* GC: Don't assume anything about obj & dst_cont, they may not be rooted.
   In the most common case of apply-continuation & continuation->applicative
   they are rooted, but in general there's no way to protect them, because
   this ends in a setjmp */
void kcall_cont(klisp_State *K, TValue dst_cont, TValue obj)
{
    krooted_tvs_push(K, dst_cont);
    krooted_tvs_push(K, obj);
    TValue src_cont = kget_cc(K);
    TValue int_ls = create_interception_list(K, src_cont, dst_cont);
    TValue new_cont;
    if (ttisnil(int_ls)) {
        new_cont = dst_cont; /* no interceptions */
    } else {
        krooted_tvs_push(K, int_ls);
        /* we have to contruct a continuation to do the interceptions
           in order and finally call dst_cont if no divert occurs */
        new_cont = kmake_continuation(K, kget_cc(K), do_interception, 
                                      2, int_ls, dst_cont);
        krooted_tvs_pop(K);
    }
    /* no more allocation from this point */
    krooted_tvs_pop(K);
    krooted_tvs_pop(K);

    /*
    ** This may come from an error detected by the interpreter, so we can't
    ** do just a return (like kapply_cc does), maybe we could somehow 
    ** differentiate to avoid the longjmp when return would suffice 
    ** TODO: do that
    */
    kset_cc(K, new_cont);
    klispS_apply_cc(K, obj);
    longjmp(K->error_jb, 1);
}

void klispS_init_repl(klisp_State *K)
{
    /* this is in krepl.c */
    kinit_repl(K);
}

void klispS_run(klisp_State *K)
{
    while(true) {
        if (setjmp(K->error_jb)) {
            /* continuation called */
            /* TEMP: do nothing, the loop will call the continuation */
        } else {
            /* all ok, continue with next func */
            while (K->next_func) {
                /* next_func is either operative or continuation
                   but in any case the call is the same */
                (*(K->next_func))(K);
            }
            /* K->next_func is NULL, this means we should exit already */
            break;
        }
    }
}
