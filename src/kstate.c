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

#include <stddef.h>
#include <setjmp.h>

#include "klisp.h"
#include "klimits.h"
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
#include "ksymbol.h"
#include "kstring.h"
#include "kport.h"
#include "ktable.h"
#include "kbytevector.h"
#include "kvector.h"

#include "kghelpers.h" /* for creating list_app */
#include "kgerrors.h" /* for creating error hierarchy */

#include "kgc.h" /* for memory freeing & gc init */


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

    K->curr_cont = KNIL;

    K->next_obj = KINERT;
    K->next_func = NULL;
    K->next_value = KINERT;
    K->next_env = KNIL;
    K->next_xparams = NULL;
    K->next_si = KNIL;

    /* these will be properly initialized later */
    K->eval_op = KINERT;
    K->list_app = KINERT;
    K->ground_env = KINERT;
    K->module_params_sym = KINERT;
    K->root_cont = KINERT;
    K->error_cont = KINERT;
    K->system_error_cont = KINERT;

    K->frealloc = f;
    K->ud = ud;

    /* current input and output */
    K->curr_port = KINERT; /* set on each call to read/write */

    /* input / output for dynamic keys */
    /* these are init later */
    K->kd_in_port_key = KINERT;
    K->kd_out_port_key = KINERT;
    K->kd_error_port_key = KINERT;

    /* strict arithmetic dynamic key */
    /* this is init later */
    K->kd_strict_arith_key = KINERT;

    /* GC */
    K->currentwhite = bit2mask(WHITE0BIT, FIXEDBIT);
    K->gcstate = GCSpause;
    K->sweepstrgc = 0;
    K->rootgc = NULL;
    K->sweepgc = &(K->rootgc);
    K->gray = NULL;
    K->grayagain = NULL;
    K->weak = NULL;
    K->tmudata = NULL;
    K->totalbytes = state_size() + KS_ISSIZE * sizeof(TValue) +
	KS_ITBSIZE;
    K->GCthreshold = UINT32_MAX; /* we still have a lot of allocation
				    to do, put a very high value to 
				    avoid collection */
    K->estimate = 0; /* doesn't matter, it is set by gc later */
    K->gcdept = 0;
    K->gcpause = KLISPI_GCPAUSE;
    K->gcstepmul = KLISPI_GCMUL;

    /* TEMP: err */
    /* do nothing for now */

    /* init the stacks used to protect variables & values from gc,
     this should be done before any new object is created because
     they are used by them */
    K->rooted_tvs_top = 0;
    K->rooted_vars_top = 0;

    /* initialize strings */

    /* initial size of string/symbol table */
    K->strt.size = 0;
    K->strt.nuse = 0;
    K->strt.hash = NULL;
    klispS_resize(K, MINSTRTABSIZE); 

    /* initialize name info table */
    /* needs weak keys, otherwise every named object would
       be fixed! */
    K->name_table = klispH_new(K, 0, MINNAMETABSIZE, 
	K_FLAG_WEAK_KEYS);
    /* here the keys are uncollectable */
    K->cont_name_table = klispH_new(K, 0, MINCONTNAMETABSIZE, 
	K_FLAG_WEAK_NOTHING);

    /* Empty string */
    /* MAYBE: fix it so we can remove empty_string from roots */
    K->empty_string = kstring_new_b_imm(K, "");

    /* Empty bytevector */
    /* MAYBE: fix it so we can remove empty_bytevector from roots */
    /* XXX: find a better way to do this */
    K->empty_bytevector = KNIL; /* trick constructor to create empty bytevector */
    K->empty_bytevector = kbytevector_new_bs_imm(K, NULL, 0);

    /* Empty vector */
    /* MAYBE: see above */
    K->empty_vector = kvector_new_bs_g(K, false, NULL, 0);

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
    K->ktok_sexp_comment = kcons(K, ch2tv(';'), KNIL);

    /* TEMP: For now just hardcode it to 8 spaces tab-stop */
    K->ktok_source_info.tab_width = 8;
    /* all three are set on each call to read */
    K->ktok_source_info.filename = KINERT; 
    K->ktok_source_info.line = 1; 
    K->ktok_source_info.col = 0;

    K->ktok_nested_comments = 0;

    ktok_init(K);

    /* initialize reader */
    K->shared_dict = KNIL;
    K->read_mconsp = false; /* set on each call to read */

    /* initialize writer */
    K->write_displayp = false; /* set on each call to write */

    /* initialize temp stack */
    K->ssize = KS_ISSIZE;
    K->stop = 0; /* stack is empty */
    K->sbuf = (TValue *)s;

    /* the dynamic ports and the keys for the dynamic ports */
    TValue in_port = kmake_std_fport(K, kstring_new_b_imm(K, "*STDIN*"),
				    false, false,  stdin);
    TValue out_port = kmake_std_fport(K, kstring_new_b_imm(K, "*STDOUT*"),
				     true, false, stdout);
    TValue error_port = kmake_std_fport(K, kstring_new_b_imm(K, "*STDERR*"),
				       true, false, stderr);
    K->kd_in_port_key = kcons(K, KTRUE, in_port);
    K->kd_out_port_key = kcons(K, KTRUE, out_port);
    K->kd_error_port_key = kcons(K, KTRUE, error_port);

    /* strict arithmetic key, (starts as false) */
    K->kd_strict_arith_key = kcons(K, KTRUE, KFALSE);

    /* create the ground environment and the eval operative */
    int32_t line_number; 
    TValue si;
    K->eval_op = kmake_operative(K, keval_ofn, 0), line_number = __LINE__;
    si = kcons(K, kstring_new_b_imm(K, __FILE__), 
		      kcons(K, i2tv(line_number), i2tv(0)));
    kset_source_info(K, K->eval_op, si);

    /* TODO: si */
    TValue eval_name = ksymbol_new_b(K, "eval", KNIL);
    ktry_set_name(K, K->eval_op, eval_name);
    
    K->list_app = kmake_applicative(K, list, 0), line_number = __LINE__;
    si = kcons(K, kstring_new_b_imm(K, __FILE__), 
		      kcons(K, i2tv(__LINE__), i2tv(0)));
    kset_source_info(K, K->list_app, si);
    kset_source_info(K, kunwrap(K->list_app), si);

    /* ground environment has a hashtable for bindings */
    K->ground_env = kmake_table_environment(K, KNIL);
//    K->ground_env = kmake_empty_environment(K);

    /* MAYBE: fix it so we can remove module_params_sym from roots */
    /* TODO si */
    K->module_params_sym = ksymbol_new_b(K, "module-parameters", KNIL);

    /* Create the root and error continuation (will be added to the 
     environment in kinit_ground_env) */
    K->root_cont = kmake_continuation(K, KNIL, do_root_exit, 0);

    #if KTRACK_SI
    /* Add source info to the cont */
    TValue str = kstring_new_b_imm(K, __FILE__);
    TValue tail = kcons(K, i2tv(__LINE__), i2tv(0));
    si = kcons(K, str, tail);
    kset_source_info(K, K->root_cont, si);
    #endif

    K->error_cont = kmake_continuation(K, K->root_cont, do_error_exit, 0);

    #if KTRACK_SI
    str = kstring_new_b_imm(K, __FILE__);
    tail = kcons(K, i2tv(__LINE__), i2tv(0));
    si = kcons(K, str, tail);
    kset_source_info(K, K->error_cont, si);
    #endif

    /* this must be done before calling kinit_ground_env */
    kinit_error_hierarchy(K); 
    kinit_ground_env(K);
    kinit_cont_names(K);

    /* create a std environment and leave it in K->next_env */
    K->next_env = kmake_table_environment(K, K->ground_env);

    /* set the threshold for gc start now that we have allocated all mem */ 
    K->GCthreshold = 4*K->totalbytes;

    return K;
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
    K->next_func = NULL;     /* force the loop to terminate */
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
inline TValue create_interception_list(klisp_State *K, TValue src_cont, 
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

void klisp_close (klisp_State *K)
{
    /* free all collectable objects */
    klispC_freeall(K);

    /* free helper buffers */
    klispM_freemem(K, ks_sbuf(K), ks_ssize(K) * sizeof(TValue));
    klispM_freemem(K, ks_tbuf(K), ks_tbsize(K));

    /* there should be no pending strings */
    klisp_assert(K->strt.nuse == 0);

    /* free string/symbol table */
    klispM_freearray(K, K->strt.hash, K->strt.size, GCObject *);

    /* only remaining mem should be of the state struct */
    klisp_assert(K->totalbytes == state_size());

    /* NOTE: this needs to be done "by hand" */
    (*(K->frealloc))(K->ud, K, state_size(), 0);
}
