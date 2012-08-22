/*
** kstate.h
** klisp vm state
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: The main structure is from Lua.
*/

#ifndef kstate_h
#define kstate_h

#include <stdio.h>
#include <setjmp.h>
#include <pthread.h>

#include "klimits.h"
#include "kobject.h"
#include "klisp.h"
#include "ktoken.h"
#include "kmem.h"

/* XXX: for now, lines and column names are fixints */
/* MAYBE: this should be in tokenizer */
typedef struct {
    TValue filename;
    int32_t tab_width;
    int32_t line;
    int32_t col;
    
    int32_t saved_line;
    int32_t saved_col;
} ksource_info_t;

/* in klisp this has both the immutable strings & the symbols */
typedef struct stringtable {
    GCObject **hash;
    uint32_t nuse;  /* number of elements */
    int32_t size;
} stringtable;

#define GC_PROTECT_SIZE 32

/* NOTE: when adding TValues here, remember to add them to
   markroot in kgc.c!! */

/* TODO split this struct in substructs (e.g. run_context, tokenizer, 
   gc, etc) */

/*
** `global state', shared by all threads of this state
*/
typedef struct global_State {
    /* Global tables */
    stringtable strt;  /* hash table for immutable strings & symbols */
    TValue name_table; /* hash tables for naming objects */
    TValue cont_name_table; /* hash tables for naming continuation functions */

    /* Memory allocator */
    klisp_Alloc frealloc;  /* function to reallocate memory */
    void *ud;            /* auxiliary data to `frealloc' */

    /* GC */
    uint16_t currentwhite; /* the one of the two whites that is in use in
                              this collection cycle */
    uint8_t gcstate;  /* state of garbage collector */
    int32_t sweepstrgc;  /* position of sweep in `strt' */
    GCObject *rootgc; /* list of all collectable objects */
    GCObject **sweepgc;  /* position of sweep in `rootgc' */
    GCObject *gray;  /* list of gray objects */
    GCObject *grayagain;  /* list of objects to be traversed atomically */
    GCObject *weak;  /* list of weak tables (to be cleared) */
    GCObject *tmudata;  /* last element of list of userdata to be GC */
    uint32_t GCthreshold;
    uint32_t totalbytes;  /* number of bytes currently allocated */
    uint32_t estimate;  /* an estimate of number of bytes actually in use */
    uint32_t gcdept;  /* how much GC is `behind schedule' */
    int32_t gcpause;  /* size of pause between successive GCs */
    int32_t gcstepmul;  /* GC `granularity' */

    /* Basic Continuation objects */
    TValue root_cont; 
    TValue error_cont;
    TValue system_error_cont;  /* initialized by kinit_error_hierarchy() */

    /* Strings */
    TValue empty_string;

    /* Bytevectors */
    TValue empty_bytevector;

    /* Vectors */
    TValue empty_vector;
    
    /* tokenizer */
    /* special tokens, see ktoken.c for rationale */
    TValue ktok_lparen;
    TValue ktok_rparen;
    TValue ktok_dot;
    TValue ktok_sexp_comment;

    /* require */
    TValue require_path;
    TValue require_table;

    /* libraries */
    TValue libraries_registry; /* this is a list, because library names
                                are list of symbols and numbers so 
                                putting them in a table isn't easy */

    /* XXX These should be changed to use thread specific storage */
    /* for current-input-port, current-output-port, current-error-port */
    TValue kd_in_port_key;
    TValue kd_out_port_key;
    TValue kd_error_port_key;

    /* for strict-arithmetic */
    TValue kd_strict_arith_key;

    /* Misc objects that are convenient to have here for now */
    TValue eval_op; /* the operative for evaluation */
    TValue list_app; /* the applicative for list evaluation */
    TValue memoize_app; /* the applicative for promise memoize */
    TValue ground_env;  /* the environment with all the ground definitions */
    /* NOTE standard environments are environments with no bindings and 
       ground_env as parent */
    TValue module_params_sym; /* this is the symbol "module-parameters" */
    /* (it is used in get-module) */
    
    /* The main thread */
    klisp_State *mainthread;
    /* The GIL (Global Interpreter Lock) */
    /* (at least for now) we'll use a non recursive mutex */
    pthread_mutex_t gil; 
} global_State;

struct klisp_State {
    CommonHeader; /* This represents a thread object */
    global_State *k_G;
    /* Current state of execution */
    TValue curr_cont; /* the current continuation of this thread */
    /*
    ** If next_env is NIL, then the next_func is from a continuation
    ** and otherwise next_func is from an operative
    */
    TValue next_obj; /* this is the operative or continuation to call
                        must be here to protect it from gc */
    klisp_CFunction next_func; /* the next function to call 
                                  (operative or continuation) */
    TValue next_value;        /* the value to be passed to the next function */
    TValue next_env; /* either NIL or an environment for next operative */
    TValue *next_xparams; 
    /* TODO replace with GCObject *next_si */
    TValue next_si; /* the source code info for this call */

    /* TEMP: error handling */
    jmp_buf error_jb;

    /* XXX all reader and writer info should be local to the current
       continuation to allow user defined port types */
    /* input/output port in use (for read & write) */
    TValue curr_port; /* save the port to update source info on errors */

    /* WORKAROUND for repl */
    bool ktok_seen_eof; /* to keep track of eofs that later dissapear */
    /* source info tracking */
    ksource_info_t ktok_source_info;
    /* TODO do this with a string or bytevector */
    /* tokenizer buffer (XXX this could be done with a string) */
    int32_t ktok_buffer_size;
    int32_t ktok_buffer_idx;
    char *ktok_buffer;

    int32_t ktok_nested_comments;

    /* reader */
    /* TODO: replace the list with a hashtable */
    TValue shared_dict;
    bool read_mconsp;

    /* writer */
    bool write_displayp;

    /* TODO do this with a vector */
    /* auxiliary stack (XXX this could be a vector) */
    int32_t ssize; /* total size of array */
    int32_t stop; /* top of the stack (all elements are below this index) */
    TValue *sbuf;

    /* These could be eliminated if a stack was adopted for the c interface */
    /* (like in lua) */
    /* TValue stack to protect values from gc, must not grow, otherwise 
       it may call the gc */
    int32_t rooted_tvs_top;
    TValue rooted_tvs_buf[GC_PROTECT_SIZE];

    /* TValue * stack to protect c variables from gc. This is used when the
       object pointed to by a variable may change */
    int32_t rooted_vars_top;
    TValue *rooted_vars_buf[GC_PROTECT_SIZE];
};

#define G(K)	(K->k_G)

/*
** Union of all Kernel heap-allocated values
*/
union GCObject {
    GCheader gch;
    MGCheader mgch;
    Pair pair;
    Symbol sym;
    String str;
    Environment env;
    Continuation cont;
    Operative op;
    Applicative app;
    Encapsulation enc;
    Promise prom;
    Table table;
    Bytevector bytevector;
    Port port; /* common fields for all types of ports */
    FPort fport;
    MPort mport;
    Vector vector;
    Keyword keyw;
    Library lib;
    klisp_State th; /* thread */
};

/* some size related macros */
#define KS_ISSIZE (1024)
#define KS_ITBSIZE (1024)

klisp_State *klispT_newthread(klisp_State *K);
void klispT_freethread(klisp_State *K, klisp_State *K1);

/*
** TEMP: for now use inlined functions, later check output in 
**   different compilers and/or profile to see if it's worthy to 
**   eliminate it, change it to compiler specific or replace it
**   with defines 
*/

/*
** Stack functions 
*/

void ks_sshrink(klisp_State *K, int32_t new_top);
void ks_sgrow(klisp_State *K, int32_t new_top);

static inline void ks_spush(klisp_State *K, TValue obj);
static inline TValue ks_spop(klisp_State *K);
/* this is for DISCARDING stack pop (value isn't used, avoid warning) */ 
#define ks_sdpop(st_) (UNUSED(ks_spop(st_)))
static inline void ks_sdiscardn(klisp_State *K, int32_t n);
static inline TValue ks_sget(klisp_State *K);
static inline void ks_sclear(klisp_State *K);
static inline bool ks_sisempty(klisp_State *K);

/* some stack manipulation macros */
#define ks_ssize(st_) ((st_)->ssize)
#define ks_stop(st_) ((st_)->stop)
#define ks_sbuf(st_) ((st_)->sbuf)
#define ks_selem(st_, i_) ((ks_sbuf(st_))[i_])

static inline void ks_spush(klisp_State *K, TValue obj)
{
    ks_selem(K, ks_stop(K)) = obj;
    ++ks_stop(K);
    /* put check after so that there is always space for one obj, and if 
       realloc is needed, obj is already rooted */
    if (ks_stop(K) == ks_ssize(K)) {
        ks_sgrow(K, ks_stop(K)+1);
    }
}


static inline TValue ks_spop(klisp_State *K)
{
    if (ks_ssize(K) != KS_ISSIZE && ks_stop(K)-1 < (ks_ssize(K) / 4))
        ks_sshrink(K, ks_stop(K)-1);
    TValue obj = ks_selem(K, ks_stop(K) - 1);
    --ks_stop(K);
    return obj;
}

static inline TValue ks_sget(klisp_State *K)
{
    return ks_selem(K, ks_stop(K) - 1);
}

static inline void ks_sdiscardn(klisp_State *K, int32_t n)
{
    int32_t new_top = ks_stop(K) - n;
    ks_stop(K) = new_top;
    if (ks_ssize(K) != KS_ISSIZE && new_top < (ks_ssize(K) / 4))
        ks_sshrink(K, new_top);
    return;
}

static inline void ks_sclear(klisp_State *K)
{
    if (ks_ssize(K) != KS_ISSIZE)
        ks_sshrink(K, 0);
    ks_stop(K) = 0;
}

static inline bool ks_sisempty(klisp_State *K)
{
    return ks_stop(K) == 0;
}

/*
** Tokenizer char buffer functions
*/
void ks_tbshrink(klisp_State *K, int32_t new_top);
void ks_tbgrow(klisp_State *K, int32_t new_top);

static inline void ks_tbadd(klisp_State *K, char ch);
#define ks_tbpush(K_, ch_) (ks_tbadd((K_), (ch_)))
static inline char ks_tbget(klisp_State *K);
static inline char ks_tbpop(klisp_State *K);
/* this is for DISCARDING stack pop (value isn't used, avoid warning) */ 
#define ks_tbdpop(st_) (UNUSED(ks_tbpop(st_)))

static inline char *ks_tbget_buffer(klisp_State *K);
static inline void ks_tbclear(klisp_State *K);
static inline bool ks_tbisempty(klisp_State *K);

/* some buf manipulation macros */
#define ks_tbsize(st_) ((st_)->ktok_buffer_size)
#define ks_tbidx(st_) ((st_)->ktok_buffer_idx)
#define ks_tbuf(st_) ((st_)->ktok_buffer)
#define ks_tbelem(st_, i_) ((ks_tbuf(st_))[i_])

static inline void ks_tbadd(klisp_State *K, char ch)
{
    if (ks_tbidx(K) == ks_tbsize(K)) 
        ks_tbgrow(K, ks_tbidx(K)+1);
    ks_tbelem(K, ks_tbidx(K)) = ch;
    ++ks_tbidx(K);
}

static inline char ks_tbget(klisp_State *K)
{
    return ks_tbelem(K, ks_tbidx(K) - 1);
}

static inline char ks_tbpop(klisp_State *K)
{
    if (ks_tbsize(K) != KS_ITBSIZE && ks_tbidx(K)-1 < (ks_tbsize(K) / 4))
        ks_tbshrink(K, ks_tbidx(K)-1);
    char ch = ks_tbelem(K, ks_tbidx(K) - 1);
    --ks_tbidx(K);
    return ch;
}

static inline char *ks_tbget_buffer(klisp_State *K)
{
    klisp_assert(ks_tbelem(K, ks_tbidx(K) - 1) == '\0');
    return ks_tbuf(K);
}

static inline void ks_tbclear(klisp_State *K)
{
    if (ks_tbsize(K) != KS_ITBSIZE)
        ks_tbshrink(K, 0);
    ks_tbidx(K) = 0;
}

static inline bool ks_tbisempty(klisp_State *K)
{
    return ks_tbidx(K) == 0;
}

/*
** Functions to protect values from GC
** TODO: add write barriers
*/
static inline void krooted_tvs_push(klisp_State *K, TValue tv)
{
    klisp_assert(K->rooted_tvs_top < GC_PROTECT_SIZE);
    K->rooted_tvs_buf[K->rooted_tvs_top++] = tv;
}

static inline void krooted_tvs_pop(klisp_State *K)
{
    klisp_assert(K->rooted_tvs_top > 0);
    --(K->rooted_tvs_top);
}

static inline void krooted_tvs_clear(klisp_State *K) { K->rooted_tvs_top = 0; }

static inline void krooted_vars_push(klisp_State *K, TValue *v)
{
    klisp_assert(K->rooted_vars_top < GC_PROTECT_SIZE);
    K->rooted_vars_buf[K->rooted_vars_top++] = v;
}

static inline void krooted_vars_pop(klisp_State *K)
{
    klisp_assert(K->rooted_vars_top > 0);
    --(K->rooted_vars_top);
}

static inline void krooted_vars_clear(klisp_State *K) { K->rooted_vars_top = 0; }

/*
** Source code tracking
** MAYBE: add source code tracking to symbols
*/
#if KTRACK_SI
static inline TValue kget_source_info(klisp_State *K, TValue obj)
{
    UNUSED(K);
    klisp_assert(khas_si(obj));
    GCObject *si = gcvalue(obj)->gch.si;
    klisp_assert(si != NULL);
    return gc2pair(si);
}

static inline void kset_source_info(klisp_State *K, TValue obj, TValue si)
{
    UNUSED(K);
    klisp_assert(kcan_have_si(obj));
    klisp_assert(ttisnil(si) || ttispair(si));
    if (ttisnil(si)) {
        gcvalue(obj)->gch.si = NULL;
        gcvalue(obj)->gch.kflags &= ~(K_FLAG_HAS_SI);
    } else {
        gcvalue(obj)->gch.si = gcvalue(si);
        gcvalue(obj)->gch.kflags |= K_FLAG_HAS_SI;
    }
}

static inline TValue ktry_get_si(klisp_State *K, TValue obj)
{
    UNUSED(K);
    return (khas_si(obj))? gc2pair(gcvalue(obj)->gch.si) : KNIL;
}

static inline TValue kget_csi(klisp_State *K)
{
    return K->next_si;
}
#endif

/*
** Functions to manipulate the current continuation and calling 
** operatives
*/
static inline void klispS_apply_cc(klisp_State *K, TValue val)
{
    /* TODO write barriers */

    /* various assert to check the freeing of gc protection methods */
    /* TODO add marks assertions */
    klisp_assert(K->rooted_tvs_top == 0);
    klisp_assert(K->rooted_vars_top == 0);

    K->next_obj = K->curr_cont; /* save it from GC */
    Continuation *cont = tv2cont(K->curr_cont);
    K->next_func = cont->fn;
    K->next_value = val;
    /* NOTE: this is needed to differentiate a return from a tail call */
    K->next_env = KNIL;
    K->next_xparams = cont->extra;
    K->curr_cont = cont->parent;
    K->next_si = ktry_get_si(K, K->next_obj);
}

#define kapply_cc(K_, val_) klispS_apply_cc((K_), (val_)); return

static inline TValue klispS_get_cc(klisp_State *K)
{
    return K->curr_cont;
}

#define kget_cc(K_) (klispS_get_cc(K_))

static inline void klispS_set_cc(klisp_State *K, TValue new_cont)
{
    K->curr_cont = new_cont;
}

#define kset_cc(K_, c_) (klispS_set_cc(K_, c_))

static inline void klispS_tail_call_si(klisp_State *K, TValue top, TValue ptree, 
                                TValue env, TValue si)
{
    /* TODO write barriers */
    
    /* various assert to check the freeing of gc protection methods */
    klisp_assert(K->rooted_tvs_top == 0);
    klisp_assert(K->rooted_vars_top == 0);

    K->next_obj = top;
    Operative *op = tv2op(top);
    K->next_func = op->fn;
    K->next_value = ptree;
    /* NOTE: this is what differentiates a tail call from a return */
    klisp_assert(ttisenvironment(env));
    K->next_env = env;
    K->next_xparams = op->extra;
    K->next_si = si;
}

#define ktail_call_si(K_, op_, p_, e_, si_)                             \
    { klispS_tail_call_si((K_), (op_), (p_), (e_), (si_)); return; }

/* if no source info is needed */
#define ktail_call(K_, op_, p_, e_)                                     \
    { klisp_State *K__ = (K_);                                          \
        TValue op__ = (op_);                                            \
        (ktail_call_si(K__, op__, p_, e_, ktry_get_si(K__, op__))); }	\

#define ktail_eval(K_, p_, e_)                              \
    { klisp_State *K__ = (K_);                              \
        TValue p__ = (p_);                                  \
        klispS_tail_call_si(K__, G(K__)->eval_op, p__, (e_),    \
                            ktry_get_si(K__, p__));			\
        return; }

/* helper for continuation->applicative & kcall_cont */
void cont_app(klisp_State *K);
void kcall_cont(klisp_State *K, TValue dst_cont, TValue obj);
void klispS_init_repl(klisp_State *K);
void klispS_run(klisp_State *K);
void klisp_close (klisp_State *K);

void do_interception(klisp_State *K);

/* for root and error continuations */
void do_root_exit(klisp_State *K);
void do_error_exit(klisp_State *K);

/* simple accessors for dynamic keys */

/* XXX: this is ugly but we can't include kpair.h here so... */
/* MAYBE: move car & cdr to kobject.h */
/* TODO: use these where appropriate */
#define kcurr_input_port(K) (tv2pair(G(K)->kd_in_port_key)->cdr)
#define kcurr_output_port(K) (tv2pair(G(K)->kd_out_port_key)->cdr)
#define kcurr_error_port(K) (tv2pair(G(K)->kd_error_port_key)->cdr)
#define kcurr_strict_arithp(K) bvalue(tv2pair(G(K)->kd_strict_arith_key)->cdr)

#endif

