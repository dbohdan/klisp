/*
** kstate.c
** klisp vm state
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: this is mostly from Lua.
*/

#include <stddef.h>
#include <setjmp.h>

#include "klisp.h"
#include "kstate.h"
#include "kobject.h"
#include "kstring.h"
#include "kpair.h"
#include "kmem.h"
#include "keval.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kcontinuation.h"
#include "kenvironment.h"
#include "kground.h"
#include "krepl.h"

/*
** State creation and destruction
*/
klisp_State *klisp_newstate (klisp_Alloc f, void *ud) {
    klisp_State *K;
    void *k = (*f)(ud, NULL, 0, state_size());
    if (k == NULL) return NULL;
    void *s = (*f)(ud, NULL, 0, KS_ISSIZE * sizeof(TValue));
    if (s == NULL) { 
	(*f)(ud, k, state_size(), 0); 
	return NULL;
    }
    void *b = (*f)(ud, NULL, 0, KS_ITBSIZE);
    if (b == NULL) {
	(*f)(ud, k, state_size(), 0); 
	(*f)(ud, s, KS_ISSIZE * sizeof(TValue), 0); 
	return NULL;
    }

    K = (klisp_State *) k;

    K->symbol_table = KNIL;
    /* TODO: create a continuation */
    K->curr_cont = KNIL;

    K->next_func = NULL;
    K->next_value = KINERT;
    K->next_env = KNIL;
    K->next_xparams = NULL;

    /* these will be properly initialized later */
    K->eval_op = KINERT;
    K->ground_env = KINERT;
    K->root_cont = KINERT;
    K->error_cont = KINERT;

    K->frealloc = f;
    K->ud = ud;

    /* current input and output */
    K->curr_in = stdin;
    K->curr_out = stdout;
    K->filename_in = "*STDIN*";
    K->filename_out = "*STDOUT*";

    /* TODO: more gc info */
    K->totalbytes = state_size() + KS_ISSIZE * sizeof(TValue) +
	KS_ITBSIZE;
    K->root_gc = NULL;

    /* TEMP: err */
    /* do nothing for now */

    /* initialize strings */
    /* Empty string */
    /* TODO: make it uncollectible */
    K->empty_string = kstring_new_empty(K);

    /* initialize tokenizer */

    /* WORKAROUND: for stdin line buffering & reading of EOF */
    K->ktok_seen_eof = false;

    ks_tbsize(K) = KS_ITBSIZE;
    ks_tbidx(K) = 0; /* buffer is empty */
    ks_tbuf(K) = (char *)b;

    /* Special Tokens */
    K->ktok_lparen = kcons(K, ch2tv('('), KNIL);
    K->ktok_rparen = kcons(K, ch2tv(')'), KNIL);
    K->ktok_dot = kcons(K, ch2tv('.'), KNIL);

    /* TEMP: For now just hardcode it to 8 spaces tab-stop */
    K->ktok_source_info.tab_width = 8;
    K->ktok_source_info.filename = "*STDIN*";
    ktok_init(K);
    ktok_reset_source_info(K);

    /* initialize reader */
    K->shared_dict = KNIL;

    /* initialize writer */

    /* initialize temp stack */
    K->ssize = KS_ISSIZE;
    K->stop = 0; /* stack is empty */
    K->sbuf = (TValue *)s;

    /* create the ground environment and the eval operative */
    K->eval_op = kmake_operative(K, KNIL, KNIL, keval_ofn, 0);
    K->ground_env = kmake_empty_environment(K);
    
    kinit_ground_env(K);

    return K;
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
/* TODO: should inline this one, is only called from one place */
TValue create_interception_list(klisp_State *K, TValue src_cont, 
				       TValue dst_cont)
{
    /* GC: root intermediate pairs */
    mark_iancestors(dst_cont);
    TValue dummy = kcons(K, KINERT, KNIL);
    TValue tail = dummy;
    TValue cont = src_cont;

    /* exit guards are from the inside to the outside, and
       selected by destination */

    /* the loop is until we find the common ancestor, that has to be marked */
    while(!kis_marked(cont)) {
	/* only inner conts have exit guards */
	if (kis_inner_cont(cont)) {
	    TValue entries = tv2cont(cont)->extra[0]; /* TODO make a macro */ 

	    TValue interceptor = select_interceptor(entries);
	    if (!ttisnil(interceptor)) {
                /* TODO make macros */
		TValue denv = tv2cont(cont)->extra[1]; 
		TValue outer = tv2cont(cont)->parent;
		TValue new_entry = kcons(K, interceptor,
					kcons(K, outer, denv));
		TValue new_pair = kcons(K, new_entry, KNIL);
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

    while(!kis_marked(cont)) {
	/* only outer conts have entry guards */
	if (kis_outer_cont(cont)) {
	    TValue entries = tv2cont(cont)->extra[0]; /* TODO make a macro */
	    TValue interceptor = select_interceptor(entries);
	    if (!ttisnil(interceptor)) {
                /* TODO make macros */
		TValue denv = tv2cont(cont)->extra[1]; 
		TValue outer = cont;
		TValue new_entry = kcons(K, interceptor,
					 kcons(K, outer, denv));
		entry_int = kcons(K, new_entry, entry_int);
	    }
	}
	cont = tv2cont(cont)->parent;
    }

    unmark_iancestors(src_cont);

    /* all interceptions collected, append the two lists and return */
    kset_cdr(tail, entry_int);
    return kcdr(dummy);
}

/* this passes the operand tree to the continuation */
void cont_app(klisp_State *K, TValue *xparams, TValue ptree, TValue denv)
{
    UNUSED(denv);
    TValue cont = xparams[0];
    /* guards and dynamic variables are handled in kcall_cont() */
    kcall_cont(K, cont, ptree);
}

void do_interception(klisp_State *K, TValue *xparams, TValue obj)
{
    /* 
    ** xparams[0]: 
    ** xparams[1]: dst cont
    */
    TValue ls = xparams[0];
    if (ttisnil(ls)) {
	/* all interceptors returned normally */
	TValue dst_cont = xparams[1];
	/* TODO: this is the same code as the standard case of
	   call_cont, merge */
	Continuation *cont = tv2cont(dst_cont);
	K->next_func = cont->fn;
	K->next_value = obj;
	/* NOTE: this is needed to differentiate a return from a tail call */
	K->next_env = KNIL;
	K->next_xparams = cont->extra;
	K->curr_cont = cont->parent;

	longjmp(K->error_jb, 1);
    } else {
	/* call the operative with the passed obj and applicative
	   for outer cont as ptree in the dynamic environment of 
	   the corresponding call to guard-continuation in the 
	   dynamic extent of the associated outer continuation */
	TValue first = kcar(ls);
	TValue op = kcar(first);
	TValue outer = kcadr(first);
	TValue denv = kcddr(first);
	TValue app = kwrap(K, kmake_operative(K, KNIL, KNIL, 
					      cont_app, 1, outer));
	TValue ptree = kcons(K, obj, kcons(K, app, KNIL));
	kset_cc(K, outer);
	ktail_call(K, op, ptree, denv);
    }
}

/* GC: should probably save the cont to retain the objects in 
   xparams in case of gc (Also useful for source code info)
   probably a new field in K called active_cont */
void kcall_cont(klisp_State *K, TValue dst_cont, TValue obj)
{
    TValue src_cont = kget_cc(K);
    TValue int_ls = create_interception_list(K, src_cont, dst_cont);
    TValue new_cont;
    if (ttisnil(int_ls)) {
	new_cont = dst_cont; /* no interceptions */
    } else {
	/* we have to contruct a continuation to do the interceptions
	   in order and finally call dst_cont if no divert occurs */
	new_cont = kmake_continuation(K, kget_cc(K), KNIL, KNIL,
				      do_interception, 2, int_ls, dst_cont);

    }
    Continuation *cont = tv2cont(new_cont);
    K->next_func = cont->fn;
    K->next_value = obj;
    /* NOTE: this is needed to differentiate a return from a tail call */
    K->next_env = KNIL;
    K->next_xparams = cont->extra;
    K->curr_cont = cont->parent;

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
		if (ttisnil(K->next_env)) {
		    /* continuation application */
		    klisp_Cfunc fn = (klisp_Cfunc) K->next_func;
		    (*fn)(K, K->next_xparams, K->next_value);
		} else {
		    /* operative calling */
		    klisp_Ofunc fn = (klisp_Ofunc) K->next_func;
		    (*fn)(K, K->next_xparams, K->next_value, K->next_env);
		}
	    }
	    break;
	}
    }
}

void klisp_close (klisp_State *K)
{
    /* free all collectable objects */
    GCObject *next = K->root_gc;

    while(next) {
	GCObject *obj = next;
	next = obj->gch.next;
	int type = gch_get_type(obj);

	switch(type) {
	case K_TPAIR:
	    klispM_free(K, (Pair *)obj);
	    break;
	case K_TSYMBOL:
	    klispM_freemem(K, obj, sizeof(Symbol)+obj->sym.size+1);
	    break;
	case K_TSTRING:
	    klispM_freemem(K, obj, sizeof(String)+obj->str.size+1);
	    break;
	case K_TENVIRONMENT:
	    klispM_free(K, (Environment *)obj);
	    break;
	case K_TCONTINUATION:
	    klispM_freemem(K, obj, sizeof(Continuation) + 
			   obj->cont.extra_size * sizeof(TValue));
	    break;
	case K_TOPERATIVE:
	    klispM_freemem(K, obj, sizeof(Operative) + 
			   obj->op.extra_size * sizeof(TValue));
	    break;
	case K_TAPPLICATIVE:
	    klispM_free(K, (Applicative *)obj);
	    break;
	default:
	    /* shouldn't happen */
	    fprintf(stderr, "Unknown GCObject type: %d\n", type);
	    abort();
	}
    }
    /* free helper buffers */
    klispM_freemem(K, ks_sbuf(K), ks_ssize(K) * sizeof(TValue));
    klispM_freemem(K, ks_tbuf(K), ks_tbsize(K));

    /* only remaining mem should be of the state struct */
    assert(K->totalbytes == state_size());

    /* NOTE: this needs to be done "by hand" */
    (*(K->frealloc))(K->ud, K, state_size(), 0);
}


