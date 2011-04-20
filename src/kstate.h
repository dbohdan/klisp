/*
** kstate.h
** klisp vm state
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: The main structure is from Lua, but because (for now)
** klisp is single threaded, only global state is provided.
*/

#ifndef kstate_h
#define kstate_h

#include <stdio.h>
#include <setjmp.h>

#include "klimits.h"
#include "kobject.h"
#include "klisp.h"
#include "ktoken.h"
#include "kmem.h"

/* XXX: for now, lines and column names are fixints */
/* MAYBE: this should be in tokenizer */
typedef struct {
    char *filename;
    int32_t tab_width;
    int32_t line;
    int32_t col;
    
    char *saved_filename;
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
struct klisp_State {
    stringtable strt;  /* hash table for immutable strings & symbols */
    TValue curr_cont;

    /*
    ** If next_env is NIL, then the next_func is of type klisp_Cfunc
    ** (from a continuation) and otherwise next_func is of type
    ** klisp_Ofunc (from an operative)
    */
    TValue next_obj; /* this is the operative or continuation to call
			must be here to protect it from gc */
    void *next_func; /* the next function to call (operative or cont) */
    TValue next_value;     /* the value to be passed to the next function */
    TValue next_env; /* either NIL or an environment for next operative */
    TValue *next_xparams; 

    TValue eval_op; /* the operative for evaluation */
    TValue list_app; /* the applicative for list evaluation */
    TValue ground_env;  /* the environment with all the ground definitions */
    /* standard environments are environments with no bindings and ground_env
       as parent */
    TValue module_params_sym; /* this is the symbol "module-parameters" */
    /* it is used in get-module */
    TValue root_cont; 
    TValue error_cont;

    klisp_Alloc frealloc;  /* function to reallocate memory */
    void *ud;         /* auxiliary data to `frealloc' */

    /* GC */
    uint16_t currentwhite; /* the one of the two whites that is in use in
			      this collection cycle */
    uint8_t gcstate;  /* state of garbage collector */
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

    /* TEMP: error handling */
    jmp_buf error_jb;

     /* standard input and output */
     /* TODO: eventually these should be ports */
    FILE *curr_in;
    FILE *curr_out;
    char *filename_in;
    char *filename_out;

    /* for current-input-port, current-output-port */
    TValue kd_in_port_key;
    TValue kd_out_port_key;

    /* Strings */
    TValue empty_string;
    
    /* tokenizer */
    /* special tokens, see ktoken.c for rationale */
    TValue ktok_lparen;
    TValue ktok_rparen;
    TValue ktok_dot;

    /* WORKAROUND for repl */
    bool ktok_seen_eof;
    ksource_info_t ktok_source_info;
    /* tokenizer buffer */
    int32_t ktok_buffer_size;
    int32_t ktok_buffer_idx;
    char *ktok_buffer;

    /* reader */
    /* TODO: replace the list with a hashtable */
    TValue shared_dict;
    bool read_mconsp;

    /* writer */
    bool write_displayp;

    /* auxiliary stack */
    int32_t ssize; /* total size of array */
    int32_t stop; /* top of the stack (all elements are below this index) */
    TValue *sbuf;

    /* TValue stack to protect values from gc, must not grow, otherwise 
       it may call the gc */
    int32_t rooted_tvs_top;
    TValue rooted_tvs_buf[GC_PROTECT_SIZE];

    /* TValue * stack to protect c variables from gc. This is used when the
       object pointed to by a variable may change */
    int32_t rooted_vars_top;
    TValue *rooted_vars_buf[GC_PROTECT_SIZE];

    /* These three are useful for constructing lists by means of set-car &
       set-cdr. The idea is that these dummy pairs start as the head of 
       the list (protecting the entire chain from GC) and at the end of the
       construction, the list is cut off from the cdr of the dummy */
    TValue dummy_pair1;
    TValue dummy_pair2;
    TValue dummy_pair3;
 };

/* some size related macros */
#define KS_ISSIZE (1024)
#define KS_ITBSIZE (1024)
#define state_size() (sizeof(klisp_State))

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

inline void ks_spush(klisp_State *K, TValue obj);
inline TValue ks_spop(klisp_State *K);
/* this is for DISCARDING stack pop (value isn't used, avoid warning) */ 
#define ks_sdpop(st_) (UNUSED(ks_spop(st_)))
inline void ks_sdiscardn(klisp_State *K, int32_t n);
inline TValue ks_sget(klisp_State *K);
inline void ks_sclear(klisp_State *K);
inline bool ks_sisempty(klisp_State *K);

/* some stack manipulation macros */
#define ks_ssize(st_) ((st_)->ssize)
#define ks_stop(st_) ((st_)->stop)
#define ks_sbuf(st_) ((st_)->sbuf)
#define ks_selem(st_, i_) ((ks_sbuf(st_))[i_])

inline void ks_spush(klisp_State *K, TValue obj)
{
    ks_selem(K, ks_stop(K)) = obj;
    ++ks_stop(K);
    /* put check after so that there is always space for one obj, and if 
       realloc is needed, obj is already rooted */
    if (ks_stop(K) == ks_ssize(K)) {
	ks_sgrow(K, ks_stop(K)+1);
    }
}


inline TValue ks_spop(klisp_State *K)
{
    if (ks_ssize(K) != KS_ISSIZE && ks_stop(K)-1 < (ks_ssize(K) / 4))
	ks_sshrink(K, ks_stop(K)-1);
    TValue obj = ks_selem(K, ks_stop(K) - 1);
    --ks_stop(K);
    return obj;
}

inline TValue ks_sget(klisp_State *K)
{
    return ks_selem(K, ks_stop(K) - 1);
}

inline void ks_sdiscardn(klisp_State *K, int32_t n)
{
    int32_t new_top = ks_stop(K) - n;
    ks_stop(K) = new_top;
    if (ks_ssize(K) != KS_ISSIZE && new_top < (ks_ssize(K) / 4))
	ks_sshrink(K, new_top);
    return;
}

inline void ks_sclear(klisp_State *K)
{
    if (ks_ssize(K) != KS_ISSIZE)
	ks_sshrink(K, 0);
    ks_stop(K) = 0;
}

inline bool ks_sisempty(klisp_State *K)
{
    return ks_stop(K) == 0;
}

/*
** Tokenizer char buffer functions
*/

void ks_tbshrink(klisp_State *K, int32_t new_top);
void ks_tbgrow(klisp_State *K, int32_t new_top);

inline void ks_tbadd(klisp_State *K, char ch);
#define ks_tbpush(K_, ch_) (ks_tbadd((K_), (ch_)))
inline char ks_tbget(klisp_State *K);
inline char ks_tbpop(klisp_State *K);
/* this is for DISCARDING stack pop (value isn't used, avoid warning) */ 
#define ks_tbdpop(st_) (UNUSED(ks_tbpop(st_)))

inline char *ks_tbget_buffer(klisp_State *K);
inline void ks_tbclear(klisp_State *K);
inline bool ks_tbisempty(klisp_State *K);

/* some buf manipulation macros */
#define ks_tbsize(st_) ((st_)->ktok_buffer_size)
#define ks_tbidx(st_) ((st_)->ktok_buffer_idx)
#define ks_tbuf(st_) ((st_)->ktok_buffer)
#define ks_tbelem(st_, i_) ((ks_tbuf(st_))[i_])

inline void ks_tbadd(klisp_State *K, char ch)
{
    if (ks_tbidx(K) == ks_tbsize(K)) 
	ks_tbgrow(K, ks_tbidx(K)+1);
    ks_tbelem(K, ks_tbidx(K)) = ch;
    ++ks_tbidx(K);
}

inline char ks_tbget(klisp_State *K)
{
    return ks_tbelem(K, ks_tbidx(K) - 1);
}

inline char ks_tbpop(klisp_State *K)
{
    if (ks_tbsize(K) != KS_ITBSIZE && ks_tbidx(K)-1 < (ks_tbsize(K) / 4))
	ks_tbshrink(K, ks_tbidx(K)-1);
    char ch = ks_tbelem(K, ks_tbidx(K) - 1);
    --ks_tbidx(K);
    return ch;
}

inline char *ks_tbget_buffer(klisp_State *K)
{
    klisp_assert(ks_tbelem(K, ks_tbidx(K) - 1) == '\0');
    return ks_tbuf(K);
}

inline void ks_tbclear(klisp_State *K)
{
    if (ks_tbsize(K) != KS_ITBSIZE)
	ks_tbshrink(K, 0);
    ks_tbidx(K) = 0;
}

inline bool ks_tbisempty(klisp_State *K)
{
    return ks_tbidx(K) == 0;
}

/*
** Functions to protect values from GC
** TODO: add write barriers
*/
inline void krooted_tvs_push(klisp_State *K, TValue tv)
{
    klisp_assert(K->rooted_tvs_top < GC_PROTECT_SIZE);
    K->rooted_tvs_buf[K->rooted_tvs_top++] = tv;
}

inline void krooted_tvs_pop(klisp_State *K)
{
    klisp_assert(K->rooted_tvs_top > 0);
    --(K->rooted_tvs_top);
}

inline void krooted_tvs_clear(klisp_State *K) { K->rooted_tvs_top = 0; }

inline void krooted_vars_push(klisp_State *K, TValue *v)
{
    klisp_assert(K->rooted_vars_top < GC_PROTECT_SIZE);
    K->rooted_vars_buf[K->rooted_vars_top++] = v;
}

inline void krooted_vars_pop(klisp_State *K)
{
    klisp_assert(K->rooted_vars_top > 0);
    --(K->rooted_vars_top);
}

inline void krooted_vars_clear(klisp_State *K) { K->rooted_vars_top = 0; }

/* dummy functions will be in kpair.h, because we can't include
   it from here */

/*
** prototypes for underlying c functions of continuations &
** operatives
*/
typedef void (*klisp_Cfunc) (klisp_State*K, TValue *ud, TValue val);
typedef void (*klisp_Ofunc) (klisp_State *K, TValue *ud, TValue ptree, 
			     TValue env);

/* XXX: this is ugly but we can't include kpair.h here so... */
/* MAYBE: move car & cdr to kobject.h */
#define kstate_car(p_) (tv2pair(p_)->car)
#define kstate_cdr(p_) (tv2pair(p_)->cdr)

/*
** Functions to manipulate the current continuation and calling 
** operatives
*/
inline void klispS_apply_cc(klisp_State *K, TValue val)
{
    /* various assert to check the freeing of gc protection methods */
    klisp_assert(K->rooted_tvs_top == 0);
    klisp_assert(K->rooted_vars_top == 0);
    klisp_assert(ttispair(K->dummy_pair1) && 
		 ttisnil(kstate_cdr(K->dummy_pair1)));
    klisp_assert(ttispair(K->dummy_pair2) && 
		 ttisnil(kstate_cdr(K->dummy_pair2)));
    klisp_assert(ttispair(K->dummy_pair3) && 
		 ttisnil(kstate_cdr(K->dummy_pair3)));

    K->next_obj = K->curr_cont; /* save it from GC */
    Continuation *cont = tv2cont(K->curr_cont);
    K->next_func = cont->fn;
    K->next_value = val;
    /* NOTE: this is needed to differentiate a return from a tail call */
    K->next_env = KNIL;
    K->next_xparams = cont->extra;
    K->curr_cont = cont->parent;
}

#define kapply_cc(K_, val_) klispS_apply_cc((K_), (val_)); return

inline TValue klispS_get_cc(klisp_State *K)
{
    return K->curr_cont;
}

#define kget_cc(K_) (klispS_get_cc(K_))

inline void klispS_set_cc(klisp_State *K, TValue new_cont)
{
    K->curr_cont = new_cont;
}

#define kset_cc(K_, c_) (klispS_set_cc(K_, c_))

inline void klispS_tail_call(klisp_State *K, TValue top, TValue ptree, 
			     TValue env)
{
    /* various assert to check the freeing of gc protection methods */
    klisp_assert(K->rooted_tvs_top == 0);
    klisp_assert(K->rooted_vars_top == 0);
    klisp_assert(ttispair(K->dummy_pair1) && 
		 ttisnil(kstate_cdr(K->dummy_pair1)));
    klisp_assert(ttispair(K->dummy_pair2) && 
		 ttisnil(kstate_cdr(K->dummy_pair2)));
    klisp_assert(ttispair(K->dummy_pair3) && 
		 ttisnil(kstate_cdr(K->dummy_pair3)));

    K->next_obj = top; /* save it from GC */
    Operative *op = tv2op(top);
    K->next_func = op->fn;
    K->next_value = ptree;
    /* NOTE: this is what differentiates a tail call from a return */
    K->next_env = env;
    K->next_xparams = op->extra;
}

#define ktail_call(K_, op_, p_, e_) \
    { klispS_tail_call((K_), (op_), (p_), (e_)); return; }

#define ktail_eval(K_, p_, e_) \
    { klisp_State *K__ = (K_); \
	klispS_tail_call(K__, K__->eval_op, (p_), (e_)); return; }

/* helper for continuation->applicative & kcall_cont */
void cont_app(klisp_State *K, TValue *xparams, TValue ptree, TValue denv);
void kcall_cont(klisp_State *K, TValue dst_cont, TValue obj);
void klispS_init_repl(klisp_State *K);
void klispS_run(klisp_State *K);
void klisp_close (klisp_State *K);

#endif

