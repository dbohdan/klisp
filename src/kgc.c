/*
** kgc.c
** Garbage Collector
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: This is almost textually from lua.
** Parts that don't apply, or don't apply yet to klisp are in comments.
*/

#include <string.h>

#include "kgc.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kport.h"
#include "imath.h"
#include "imrat.h"
#include "ktable.h"
#include "kstring.h"
#include "kerror.h"

#define GCSTEPSIZE	1024u
#define GCSWEEPMAX	40
#define GCSWEEPCOST	10
#define GCFINALIZECOST	100 /* klisp: NOT USED YET */



#define maskmarks	cast(uint16_t, ~(bitmask(BLACKBIT)|WHITEBITS))

#define makewhite(g,x)							\
    ((x)->gch.gct = cast(uint16_t,					\
			 ((x)->gch.gct & maskmarks) | klispC_white(g)))

#define white2gray(x)	reset2bits((x)->gch.gct, WHITE0BIT, WHITE1BIT)
#define black2gray(x)	resetbit((x)->gch.gct, BLACKBIT)

/* NOTE: klisp strings, unlike the lua counterparts are not values,
   so they are marked as other objects */

/* klisp: NOT USED YET */
#define isfinalized(u)		testbit((u)->gct, FINALIZEDBIT)
#define markfinalized(u)	l_setbit((u)->gct, FINALIZEDBIT)

/* klisp: NOT USED YET */
#define KEYWEAK         bitmask(KEYWEAKBIT)
#define VALUEWEAK       bitmask(VALUEWEAKBIT)

/* this one is klisp specific */
#define markvaluearray(k, a, s) ({			\
	    TValue *array_ = (a);			\
    int32_t size_ = (s);				\
    for(int32_t i_ = 0; i_ < size_; i_++, array_++) {	\
	TValue mva_obj_ = *array_;			\
	markvalue(k, mva_obj_);				\
    }})

#define markvalue(k,o) { checkconsistency(o);		    \
	if (iscollectable(o) && iswhite(gcvalue(o)))	    \
	    reallymarkobject(k,gcvalue(o)); }

#define markobject(k,t) { if (iswhite(obj2gco(t)))	\
	    reallymarkobject(k, obj2gco(t)); }


#define setthreshold(g)  (g->GCthreshold = (g->estimate/100) * g->gcpause)

static void removeentry (Node *n) {
    klisp_assert(ttisfree(gval(n)));
    if (iscollectable(gkey(n)->this))/* dead key; remove it */
	gkey(n)->this = gc2deadkey(gcvalue(gkey(n)->this));
}

static void reallymarkobject (klisp_State *K, GCObject *o) 
{
    klisp_assert(iswhite(o) && !isdead(K, o));
    white2gray(o);
    /* klisp: most of klisp have the same structure, but conserve the switch
       just in case. */
    uint8_t type = o->gch.tt;
    switch (type) {
/* klisp: keep this around just in case we add it later */
#if 0
    case LUA_TUSERDATA: {
	Table *mt = gco2u(o)->metatable;
	gray2black(o);  /* udata are never gray */
	if (mt) markobject(g, mt);
	markobject(g, gco2u(o)->env);
	return;
    }
#endif
    case K_TBIGRAT: /* the n & d are copied in the bigrat, not pointed to */
    case K_TBIGINT:
	gray2black(o);  /* bigint & bigrats are never gray */
	break;
    case K_TPAIR:
    case K_TSYMBOL:
    case K_TSTRING:
    case K_TENVIRONMENT:
    case K_TCONTINUATION:
    case K_TOPERATIVE:
    case K_TAPPLICATIVE:
    case K_TENCAPSULATION:
    case K_TPROMISE:
    case K_TPORT:
    case K_TTABLE:
    case K_TERROR:
	o->gch.gclist = K->gray;
	K->gray = o;
	break;
    default:
	/* shouldn't happen */
	fprintf(stderr, "Unknown GCObject type (in GC mark): %d\n", type);
	abort();
    }
}


/* klisp: keep this around just in case we add it later */
#if 0
static void marktmu (global_State *g) {
    GCObject *u = g->tmudata;
    if (u) {
	do {
	    u = u->gch.next;
	    makewhite(g, u);  /* may be marked, if left from previous GC */
	    reallymarkobject(g, u);
	} while (u != g->tmudata);
    }
}

/* move `dead' udata that need finalization to list `tmudata' */
size_t klispC_separateudata (lua_State *L, int all) {
    global_State *g = G(L);
    size_t deadmem = 0;
    GCObject **p = &g->mainthread->next;
    GCObject *curr;
    while ((curr = *p) != NULL) {
	if (!(iswhite(curr) || all) || isfinalized(gco2u(curr)))
	    p = &curr->gch.next;  /* don't bother with them */
	else if (fasttm(L, gco2u(curr)->metatable, TM_GC) == NULL) {
	    markfinalized(gco2u(curr));  /* don't need finalization */
	    p = &curr->gch.next;
	}
	else {  /* must call its gc method */
	    deadmem += sizeudata(gco2u(curr));
	    markfinalized(gco2u(curr));
	    *p = curr->gch.next;
	    /* link `curr' at the end of `tmudata' list */
	    if (g->tmudata == NULL)  /* list is empty? */
                /* creates a circular list */
		g->tmudata = curr->gch.next = curr;  
	    else {
		curr->gch.next = g->tmudata->gch.next;
		g->tmudata->gch.next = curr;
		g->tmudata = curr;
	    }
	}
    }
    return deadmem;
}

#endif

static int32_t traversetable (klisp_State *K, Table *h) {
    int32_t i;
    TValue tv = gc2table(h);
    int32_t weakkey = ktable_has_weak_keys(tv)? 1 : 0;
    int32_t weakvalue = ktable_has_weak_values(tv)? 1 : 0;

    if (weakkey || weakvalue) {  /* is really weak? */
	h->gct &= ~(KEYWEAK | VALUEWEAK);  /* clear bits */
	h->gct |= cast(uint16_t, (weakkey << KEYWEAKBIT) |
		       (weakvalue << VALUEWEAKBIT));
	h->gclist = K->weak;  /* must be cleared after GC, ... */
	K->weak = obj2gco(h);  /* ... so put in the appropriate list */
    }
    if (weakkey && weakvalue) return 1;
    if (!weakvalue) {
	i = h->sizearray;
	while (i--)
	    markvalue(K, h->array[i]);
    }
    i = sizenode(h);
    while (i--) {
	Node *n = gnode(h, i);
	klisp_assert(ttype(gkey(n)->this) != K_TDEADKEY || 
		     ttisfree(gval(n)));
	if (ttisfree(gval(n)))
	    removeentry(n);  /* remove empty entries */
	else {
	    klisp_assert(!ttisfree(gkey(n)->this));
	    if (!weakkey) markvalue(K, gkey(n)->this);
	    if (!weakvalue) markvalue(K, gval(n));
	}
    }
    return weakkey || weakvalue;
}

#if 0
/*
** All marks are conditional because a GC may happen while the
** prototype is still being created
*/
static void traverseproto (global_State *g, Proto *f) {
    int i;
    if (f->source) stringmark(f->source);
    for (i=0; i<f->sizek; i++)  /* mark literals */
	markvalue(g, &f->k[i]);
    for (i=0; i<f->sizeupvalues; i++) {  /* mark upvalue names */
	if (f->upvalues[i])
	    stringmark(f->upvalues[i]);
    }
    for (i=0; i<f->sizep; i++) {  /* mark nested protos */
	if (f->p[i])
	    markobject(g, f->p[i]);
    }
    for (i=0; i<f->sizelocvars; i++) {  /* mark local-variable names */
	if (f->locvars[i].varname)
	    stringmark(f->locvars[i].varname);
    }
}

#endif

/*
** traverse one gray object, turning it to black.
** Returns `quantity' traversed.
*/
static int32_t propagatemark (klisp_State *K) {
    GCObject *o = K->gray;
    K->gray = o->gch.gclist;
    klisp_assert(isgray(o));
    gray2black(o);
    uint8_t type = o->gch.tt;

    switch (type) {
/*    case K_TBIGRAT: 
      case K_TBIGINT: bigints & bigrats are never gray */
    case K_TPAIR: {
	Pair *p = cast(Pair *, o);
	markvalue(K, p->mark);
	markvalue(K, p->car);
	markvalue(K, p->cdr);
	return sizeof(Pair);
    }
    case K_TSYMBOL: {
	Symbol *s = cast(Symbol *, o);
	markvalue(K, s->mark);
	markvalue(K, s->str);
	return sizeof(Symbol);
    }
    case K_TSTRING: {
	String *s = cast(String *, o);
	markvalue(K, s->mark); 
	return sizeof(String) + s->size * sizeof(char);
    }
    case K_TENVIRONMENT: {
	Environment *e = cast(Environment *, o);
	markvalue(K, e->mark); 
	markvalue(K, e->parents); 
	markvalue(K, e->bindings); 
	markvalue(K, e->keyed_node); 
	markvalue(K, e->keyed_parents); 
	return sizeof(Environment);
    }
    case K_TCONTINUATION: {
	Continuation *c = cast(Continuation *, o);
	markvalue(K, c->mark);
	markvalue(K, c->parent);
	markvalue(K, c->comb);
	markvaluearray(K, c->extra, c->extra_size);
	return sizeof(Continuation) + sizeof(TValue) * c->extra_size;
    }
    case K_TOPERATIVE: {
	Operative *op = cast(Operative *, o);
	markvaluearray(K, op->extra, op->extra_size);
	return sizeof(Operative) + sizeof(TValue) * op->extra_size;
    }
    case K_TAPPLICATIVE: {
	Applicative *a = cast(Applicative *, o);
	markvalue(K, a->underlying);
	return sizeof(Applicative);
    }
    case K_TENCAPSULATION: {
	Encapsulation *e = cast(Encapsulation *, o);
	markvalue(K, e->key);
	markvalue(K, e->value);
	return sizeof(Encapsulation);
    }
    case K_TPROMISE: {
	Promise *p = cast(Promise *, o);
	markvalue(K, p->node);
	return sizeof(Promise);
    }
    case K_TPORT: {
	Port *p = cast(Port *, o);
	markvalue(K, p->filename);
	return sizeof(Port);
    }
    case K_TTABLE: {
	Table *h = cast(Table *, o);
	if (traversetable(K, h))  /* table is weak? */
	    black2gray(o);  /* keep it gray */
	return sizeof(Table) + sizeof(TValue) * h->sizearray +
	    sizeof(Node) * sizenode(h);
    }
    case K_TERROR: {
	Error *e = cast(Error *, o);
	markvalue(K, e->who);
	markvalue(K, e->cont);
	markvalue(K, e->msg);
	markvalue(K, e->irritants);
	return sizeof(Error);
    }
    default: 
	fprintf(stderr, "Unknown GCObject type (in GC propagate): %d\n", 
		type);
	abort();
    }
}


static size_t propagateall (klisp_State *K) {
    size_t m = 0;
    while (K->gray) m += propagatemark(K);
    return m;
}

/*
** The next function tells whether a key or value can be cleared from
** a weak table. Non-collectable objects are never removed from weak
** tables. Strings behave as `values', so are never removed too. for
** other objects: if really collected, cannot keep them; for userdata
** being finalized, keep them in keys, but not in values
*/
static int32_t iscleared (TValue o, int iskey) {
    if (!iscollectable(o)) return 0;
#if 0 /* klisp: strings may be mutable... */
    if (ttisstring(o)) {
	stringmark(rawtsvalue(o));  /* strings are `values', so are never weak */
	return 0;
    }
#endif
    return iswhite(gcvalue(o));

/* klisp: keep around for later
	|| (ttisuserdata(o) && (!iskey && isfinalized(uvalue(o))));
*/
}


/*
** clear collected entries from weaktables
*/
static void cleartable (GCObject *l) {
    while (l) {
	Table *h = (Table *) (l);
	int32_t i = h->sizearray;
	klisp_assert(testbit(h->gct, VALUEWEAKBIT) ||
		     testbit(h->gct, KEYWEAKBIT));
	if (testbit(h->gct, VALUEWEAKBIT)) {
	    while (i--) {
		TValue *o = &h->array[i];
		if (iscleared(*o, 0))  /* value was collected? */
		    *o = KFREE;  /* remove value */
	    }
	}
	i = sizenode(h);
	while (i--) {
	    Node *n = gnode(h, i);
	    if (!ttisfree(gval(n)) &&  /* non-empty entry? */
		  (iscleared(key2tval(n), 1) || iscleared(gval(n), 0))) {
		gval(n) = KFREE;  /* remove value ... */
		removeentry(n);  /* remove entry from table */
	    }
	}
	l = h->gclist;
    }
}

static void freeobj (klisp_State *K, GCObject *o) {
    /* TODO use specific functions like in bigint, bigrat & table */
    uint8_t type = o->gch.tt;
    switch (type) {
    case K_TBIGINT: {
	mp_int_free(K, (Bigint *)o);
	break;
    }
    case K_TBIGRAT: {
	mp_rat_free(K, (Bigrat *)o);
	break;
    }
    case K_TPAIR:
	klispM_free(K, (Pair *)o);
	break;
    case K_TSYMBOL:
	/* symbols are in the string/symbol table */
	/* The string will be freed before/after */
	K->strt.nuse--;
	klispM_free(K, (Symbol *)o);
	break;
    case K_TSTRING:
	/* immutable strings are in the string/symbol table */
	if (kstring_immutablep(gc2str(o)))
	    K->strt.nuse--;
	klispM_freemem(K, o, sizeof(String)+o->str.size+1);
	break;
    case K_TENVIRONMENT:
	klispM_free(K, (Environment *)o);
	break;
    case K_TCONTINUATION:
	klispM_freemem(K, o, sizeof(Continuation) + 
		       o->cont.extra_size * sizeof(TValue));
	break;
    case K_TOPERATIVE:
	klispM_freemem(K, o, sizeof(Operative) + 
		       o->op.extra_size * sizeof(TValue));
	break;
    case K_TAPPLICATIVE:
	klispM_free(K, (Applicative *)o);
	break;
    case K_TENCAPSULATION:
	klispM_free(K, (Encapsulation *)o);
	break;
    case K_TPROMISE:
	klispM_free(K, (Promise *)o);
	break;
    case K_TPORT:
	/* first close the port to free the FILE structure.
	   This works even if the port was already closed,
	   it is important that this don't throw errors, because
	   the mechanism used in error handling would crash at this
	   point */
	kclose_port(K, gc2port(o));
	klispM_free(K, (Port *)o);
	break;
    case K_TTABLE:
	klispH_free(K, (Table *)o);
	break;
    case K_TERROR:
	klispE_free(K, (Error *)o);
	break;
    default:
	/* shouldn't happen */
	fprintf(stderr, "Unknown GCObject type (in GC free): %d\n", 
		type);
	abort();
    }
}


/* klisp can't have more than 4gb */
#define sweepwholelist(K,p)	sweeplist(K,p,UINT32_MAX)


static GCObject **sweeplist (klisp_State *K, GCObject **p, uint32_t count) 
{
    GCObject *curr;
    int deadmask = otherwhite(K);
    while ((curr = *p) != NULL && count-- > 0) {
	if ((curr->gch.gct ^ WHITEBITS) & deadmask) {  /* not dead? */
	    klisp_assert(!isdead(K, curr) || testbit(curr->gch.gct, FIXEDBIT));
	    makewhite(K, curr);  /* make it white (for next cycle) */
	    p = &curr->gch.next;
	} else {  /* must erase `curr' */
	    klisp_assert(isdead(K, curr) || deadmask == bitmask(SFIXEDBIT));
	    *p = curr->gch.next;
	    if (curr == K->rootgc)  /* is the first element of the list? */
		K->rootgc = curr->gch.next;  /* adjust first */
	    freeobj(K, curr);
	}
    }
    return p;
}

static void checkSizes (klisp_State *K) {
    /* check size of string/symbol hash */
    if (K->strt.nuse < cast(uint32_t , K->strt.size/4) &&
	    K->strt.size > MINSTRTABSIZE*2)
	klispS_resize(K, K->strt.size/2);  /* table is too big */
#if 0 /* not used in klisp */
    /* check size of buffer */
    if (luaZ_sizebuffer(&g->buff) > LUA_MINBUFFER*2) {  /* buffer too big? */
	size_t newsize = luaZ_sizebuffer(&g->buff) / 2;
	luaZ_resizebuffer(L, &g->buff, newsize);
    }
#endif
}

#if 0 /* klisp: keep this around */
static void GCTM (lua_State *L) {
    global_State *g = G(L);
    GCObject *o = g->tmudata->gch.next;  /* get first element */
    Udata *udata = rawgco2u(o);
    const TValue *tm;
    /* remove udata from `tmudata' */
    if (o == g->tmudata)  /* last element? */
	g->tmudata = NULL;
    else
	g->tmudata->gch.next = udata->uv.next;
    udata->uv.next = g->mainthread->next;  /* return it to `root' list */
    g->mainthread->next = o;
    makewhite(g, o);
    tm = fasttm(L, udata->uv.metatable, TM_GC);
    if (tm != NULL) {
	lu_byte oldah = L->allowhook;
	lu_mem oldt = g->GCthreshold;
	L->allowhook = 0;  /* stop debug hooks during GC tag method */
	g->GCthreshold = 2*g->totalbytes;  /* avoid GC steps */
	setobj2s(L, L->top, tm);
	setuvalue(L, L->top+1, udata);
	L->top += 2;
	luaD_call(L, L->top - 2, 0);
	L->allowhook = oldah;  /* restore hooks */
	g->GCthreshold = oldt;  /* restore threshold */
    }
}


/*
** Call all GC tag methods
*/
void klispC_callGCTM (lua_State *L) {
    while (G(L)->tmudata)
	GCTM(L);
}
#endif

/* This still leaves allocated objs in K, namely the 
   arrays that aren't TValues */
void klispC_freeall (klisp_State *K) {
    /* mask to collect all elements */
    K->currentwhite = WHITEBITS | bitmask(SFIXEDBIT); /* in klisp this may not be
							 necessary */
    sweepwholelist(K, &K->rootgc);
    /* free all symbol/string lists */
    for (int32_t i = 0; i < K->strt.size; i++)  
	sweepwholelist(K, &K->strt.hash[i]);
}


/* mark root set */
static void markroot (klisp_State *K) {
    K->gray = NULL;
    K->grayagain = NULL; 
    K->weak = NULL; 

    /* TEMP: this is quite awfull, think of other way to do this */
    /* MAYBE: some of these could be FIXED */
    markvalue(K, K->name_table);
    markvalue(K, K->si_table);
    markvalue(K, K->curr_cont);
    markvalue(K, K->next_obj);
    markvalue(K, K->next_value);
    markvalue(K, K->next_env);
    markvalue(K, K->next_si);
    /* NOTE: next_x_params is protected by next_obj */
    markvalue(K, K->eval_op);
    markvalue(K, K->list_app);
    markvalue(K, K->ground_env);
    markvalue(K, K->module_params_sym);
    markvalue(K, K->root_cont);
    markvalue(K, K->error_cont);

    markvalue(K, K->kd_in_port_key);
    markvalue(K, K->kd_out_port_key);
    markvalue(K, K->empty_string);

    markvalue(K, K->ktok_lparen);
    markvalue(K, K->ktok_rparen);
    markvalue(K, K->ktok_dot);
    markvalue(K, K->shared_dict);

    /* Mark all objects in the auxiliary stack,
       (all valid indexes are below top), all the objects in
       the two protected areas, and the three dummy pairs */
    markvaluearray(K, K->sbuf, K->stop);
    markvaluearray(K, K->rooted_tvs_buf, K->rooted_tvs_top);
    /* the area protecting variables is an array of type TValue *[] */
    TValue **ptr = K->rooted_vars_buf;
    for (int i = 0, top = K->rooted_vars_top; i < top; i++, ptr++) {
	markvalue(K, **ptr);
    }
    
    markvalue(K, K->dummy_pair1);
    markvalue(K, K->dummy_pair2);
    markvalue(K, K->dummy_pair3);
    K->gcstate = GCSpropagate;
}

static void atomic (klisp_State *K) {
    size_t udsize;  /* total size of userdata to be finalized */
    /* traverse objects caught by write barrier */
    propagateall(K);

    /* remark weak tables */
    K->gray = K->weak; 
    K->weak = NULL;
    propagateall(K);

    /* remark gray again */
    K->gray = K->grayagain;
    K->grayagain = NULL;
    propagateall(K);

    udsize = 0; /* to init var 'till we add user data */
#if 0 /* keep around */
    udsize = klispC_separateudata(L, 0);  /* separate userdata to be finalized */
    marktmu(g);  /* mark `preserved' userdata */
    udsize += propagateall(g);  /* remark, to propagate `preserveness' */
#endif
    cleartable(K->weak);  /* remove collected objects from weak tables */

    /* flip current white */
    K->currentwhite = cast(uint16_t, otherwhite(K));
    K->sweepstrgc = 0;
    K->sweepgc = &K->rootgc;
    K->gcstate = GCSsweepstring;
    K->estimate = K->totalbytes - udsize;  /* first estimate */
}


static int32_t singlestep (klisp_State *K) {
    switch (K->gcstate) {
    case GCSpause: {
	markroot(K);  /* start a new collection */
	return 0;
    }
    case GCSpropagate: {
	if (K->gray)
	    return propagatemark(K);
	else {  /* no more `gray' objects */
	    atomic(K);  /* finish mark phase */
	    return 0;
	}
    }
    case GCSsweepstring: {
	uint32_t old = K->totalbytes;
	sweepwholelist(K, &K->strt.hash[K->sweepstrgc++]);
	if (K->sweepstrgc >= K->strt.size)  /* nothing more to sweep? */
	    K->gcstate = GCSsweep;  /* end sweep-string phase */
	klisp_assert(old >= K->totalbytes);
	K->estimate -= old - K->totalbytes;
	return GCSWEEPCOST;
    }
    case GCSsweep: {
	uint32_t old = K->totalbytes;
	K->sweepgc = sweeplist(K, K->sweepgc, GCSWEEPMAX);
	if (*K->sweepgc == NULL) {  /* nothing more to sweep? */
	    checkSizes(K);
	    K->gcstate = GCSfinalize;  /* end sweep phase */
	}
	klisp_assert(old >= K->totalbytes);
	K->estimate -= old - K->totalbytes;
	return GCSWEEPMAX*GCSWEEPCOST;
    }
    case GCSfinalize: {
#if 0 /* keep around */
	if (g->tmudata) {
	    GCTM(L);
	    if (g->estimate > GCFINALIZECOST)
		g->estimate -= GCFINALIZECOST;
	    return GCFINALIZECOST;
	}
	else {
#endif
	    K->gcstate = GCSpause;  /* end collection */
	    K->gcdept = 0;
	    return 0;
#if 0
	}
#endif
    }
    default: klisp_assert(0); return 0;
    }
}


void klispC_step (klisp_State *K) {
    int32_t lim = (GCSTEPSIZE/100) * K->gcstepmul;

    if (lim == 0)
	lim = (UINT32_MAX-1)/2;  /* no limit */

    K->gcdept += K->totalbytes - K->GCthreshold;

    do {
	lim -= singlestep(K);
	if (K->gcstate == GCSpause)
	    break;
    } while (lim > 0);

    if (K->gcstate != GCSpause) {
	if (K->gcdept < GCSTEPSIZE) {
	    K->GCthreshold = K->totalbytes + GCSTEPSIZE; 
            /* - lim/g->gcstepmul;*/     
	} else {
	    K->gcdept -= GCSTEPSIZE;
	    K->GCthreshold = K->totalbytes;
	}
    } else {
	klisp_assert(K->totalbytes >= K->estimate);
	setthreshold(K);
    }
}

void klispC_fullgc (klisp_State *K) {
     if (K->gcstate <= GCSpropagate) {
	 /* reset sweep marks to sweep all elements (returning them to white) */
	 K->sweepstrgc = 0;
	 K->sweepgc = &K->rootgc;
	 /* reset other collector lists */
	 K->gray = NULL;
	 K->grayagain = NULL;
	 K->weak = NULL;
	 K->gcstate = GCSsweepstring;
     }
     klisp_assert(K->gcstate != GCSpause && K->gcstate != GCSpropagate);
     /* finish any pending sweep phase */
     while (K->gcstate != GCSfinalize) {
	 klisp_assert(K->gcstate == GCSsweepstring || K->gcstate == GCSsweep);
	 singlestep(K);
     }
     markroot(K);
     while (K->gcstate != GCSpause) {
	 singlestep(K);
     }
     setthreshold(K);
}

/* TODO: make all code using mutation to call these,
 this is actually the only thing that is missing for an incremental 
 garbage collector!
 IMPORTANT: a call to maybe a different but similar function should be
 made before assigning to a GC guarded variable, or pushed in a GC
guarded stack! */
void klispC_barrierf (klisp_State *K, GCObject *o, GCObject *v) {
    klisp_assert(isblack(o) && iswhite(v) && !isdead(K, v) && !isdead(K, o));
    klisp_assert(K->gcstate != GCSfinalize && K->gcstate != GCSpause);
    klisp_assert(o->gch.tt != K_TTABLE);
    /* must keep invariant? */
    if (K->gcstate == GCSpropagate)
	reallymarkobject(K, v);  /* restore invariant */
    else  /* don't mind */
	makewhite(K, o);  /* mark as white just to avoid other barriers */
}

void klispC_barrierback (klisp_State *K, Table *t) {
    GCObject *o = obj2gco(t);
    klisp_assert(isblack(o) && !isdead(K, o));
    klisp_assert(K->gcstate != GCSfinalize && K->gcstate != GCSpause);
    black2gray(o);  /* make table gray (again) */
    t->gclist = K->grayagain;
    K->grayagain = o;
}

/* NOTE: kflags is added for klisp */
void klispC_link (klisp_State *K, GCObject *o, uint8_t tt, uint8_t kflags) {
    o->gch.next = K->rootgc;
    K->rootgc = o;
    o->gch.gct = klispC_white(K);
    o->gch.tt = tt;
    o->gch.kflags = kflags;
    /* NOTE that o->gch.gclist doesn't need to be setted */
}

