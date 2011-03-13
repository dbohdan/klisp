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

void kcall_cont(klisp_State *K, TValue dst_cont, TValue obj)
{
    /* TODO: interceptions */
    Continuation *cont = tv2cont(dst_cont);
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


