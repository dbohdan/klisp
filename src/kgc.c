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
	TValue o_ = *array_;				\
	markvalue(k, o_);				\
    }})

#define markvalue(k,o) { checkconsistency(o);				\
	if (iscollectable(o) && iswhite(gcvalue(o)))			\
	    reallymarkobject(k,gcvalue(o)); }

#define markobject(k,t) { if (iswhite(obj2gco(t)))	\
	    reallymarkobject(k, obj2gco(t)); }


#define setthreshold(g)  (g->GCthreshold = (g->estimate/100) * g->gcpause)

/* klisp no need for it yet */
#if 0
static void removeentry (Node *n) {
    klisp_assert(ttisnil(gval(n)));
    if (iscollectable(gkey(n)))
	setttype(gkey(n), LUA_TDEADKEY);  /* dead key; remove it */
}
#endif

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
    case K_TBIGINT:
	gray2black(o);  /* bigint are never gray */
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


static int traversetable (global_State *g, Table *h) {
    int i;
    int weakkey = 0;
    int weakvalue = 0;
    const TValue *mode;
    if (h->metatable)
	markobject(g, h->metatable);
    mode = gfasttm(g, h->metatable, TM_MODE);
    if (mode && ttisstring(mode)) {  /* is there a weak mode? */
	weakkey = (strchr(svalue(mode), 'k') != NULL);
	weakvalue = (strchr(svalue(mode), 'v') != NULL);
	if (weakkey || weakvalue) {  /* is really weak? */
	    h->gct &= ~(KEYWEAK | VALUEWEAK);  /* clear bits */
	    h->gct |= cast(uint16_t, (weakkey << KEYWEAKBIT) |
			   (weakvalue << VALUEWEAKBIT));
	    h->gclist = g->weak;  /* must be cleared after GC, ... */
	    g->weak = obj2gco(h);  /* ... so put in the appropriate list */
	}
    }
    if (weakkey && weakvalue) return 1;
    if (!weakvalue) {
	i = h->sizearray;
	while (i--)
	    markvalue(g, &h->array[i]);
    }
    i = sizenode(h);
    while (i--) {
	Node *n = gnode(h, i);
	klisp_assert(ttype(gkey(n)) != LUA_TDEADKEY || ttisnil(gval(n)));
	if (ttisnil(gval(n)))
	    removeentry(n);  /* remove empty entries */
	else {
	    klisp_assert(!ttisnil(gkey(n)));
	    if (!weakkey) markvalue(g, gkey(n));
	    if (!weakvalue) markvalue(g, gval(n));
	}
    }
    return weakkey || weakvalue;
}


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
    klisp_assert(isgray(o));
    gray2black(o);
    uint8_t type = o->gch.tt;

    switch (type) {
#if 0 /* klisp: keep around */
    case LUA_TTABLE: {
	Table *h = gco2h(o);
	K->gray = h->gclist;
	if (traversetable(K, h))  /* table is weak? */
	    black2gray(o);  /* keep it gray */
	return sizeof(Table) + sizeof(TValue) * h->sizearray +
	    sizeof(Node) * sizenode(h);
    }
#endif
/*    case K_TBIGINT: bigints are never gray */
    case K_TPAIR: {
	Pair *p = cast(Pair *, o);
	markvalue(K, p->mark);
	markvalue(K, p->car);
	markvalue(K, p->cdr);
	markvalue(K, p->si);
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
	markvalue(K, c->name);
	markvalue(K, c->si);
	markvalue(K, c->parent);
	markvaluearray(K, c->extra, c->extra_size);
	return sizeof(Continuation) + sizeof(TValue) * c->extra_size;
    }
    case K_TOPERATIVE: {
	Operative *op = cast(Operative *, o);
	markvalue(K, op->name);
	markvalue(K, op->si);
	markvaluearray(K, op->extra, op->extra_size);
	return sizeof(Operative) + sizeof(TValue) * op->extra_size;
    }
    case K_TAPPLICATIVE: {
	Applicative *a = cast(Applicative *, o);
	markvalue(K, a->name);
	markvalue(K, a->si);
	markvalue(K, a->underlying);
	return sizeof(Applicative);
    }
    case K_TENCAPSULATION: {
	Encapsulation *e = cast(Encapsulation *, o);
	markvalue(K, e->name);
	markvalue(K, e->si);
	markvalue(K, e->key);
	markvalue(K, e->value);
	return sizeof(Encapsulation);
    }
    case K_TPROMISE: {
	Promise *p = cast(Promise *, o);
	markvalue(K, p->name);
	markvalue(K, p->si);
	markvalue(K, p->node);
	return sizeof(Promise);
    }
    case K_TPORT: {
	Port *p = cast(Port *, o);
	markvalue(K, p->name);
	markvalue(K, p->si);
	markvalue(K, p->filename);
	return sizeof(Port);
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

#if 0 /* klisp: keep around */
/*
** The next function tells whether a key or value can be cleared from
** a weak table. Non-collectable objects are never removed from weak
** tables. Strings behave as `values', so are never removed too. for
** other objects: if really collected, cannot keep them; for userdata
** being finalized, keep them in keys, but not in values
*/
static int iscleared (const TValue *o, int iskey) {
    if (!iscollectable(o)) return 0;
    if (ttisstring(o)) {
	stringmark(rawtsvalue(o));  /* strings are `values', so are never weak */
	return 0;
    }
    return iswhite(gcvalue(o)) ||
	(ttisuserdata(o) && (!iskey && isfinalized(uvalue(o))));
}


/*
** clear collected entries from weaktables
*/
static void cleartable (GCObject *l) {
    while (l) {
	Table *h = gco2h(l);
	int i = h->sizearray;
	klisp_assert(testbit(h->gct, VALUEWEAKBIT) ||
		     testbit(h->gct, KEYWEAKBIT));
	if (testbit(h->gct, VALUEWEAKBIT)) {
	    while (i--) {
		TValue *o = &h->array[i];
		if (iscleared(o, 0))  /* value was collected? */
		    setnilvalue(o);  /* remove value */
	    }
	}
	i = sizenode(h);
	while (i--) {
	    Node *n = gnode(h, i);
	    if (!ttisnil(gval(n)) &&  /* non-empty entry? */
		(iscleared(key2tval(n), 1) || iscleared(gval(n), 0))) {
		setnilvalue(gval(n));  /* remove value ... */
		removeentry(n);  /* remove entry from table */
	    }
	}
	l = h->gclist;
    }
}
#endif

static void freeobj (klisp_State *K, GCObject *o) {
    /* TODO use specific functions like in bigint & lua */
    uint8_t type = o->gch.tt;
    switch (type) {
	/* case LUA_TTABLE: luaH_free(L, gco2h(o)); break; */
    case K_TBIGINT: {
	mp_int_free(K, (Bigint *)o);
	break;
    }
    case K_TPAIR:
	klispM_free(K, (Pair *)o);
	break;
    case K_TSYMBOL:
	/* The string will be freed before/after */
	klispM_free(K, (Symbol *)o);
	break;
    case K_TSTRING:
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

#if 0 /* klisp: keep this around */
static void checkSizes (lua_State *L) {
    global_State *g = G(L);
    /* check size of string hash */
    if (g->strt.nuse < cast(lu_int32, g->strt.size/4) &&
	g->strt.size > MINSTRTABSIZE*2)
	luaS_resize(L, g->strt.size/2);  /* table is too big */
    /* check size of buffer */
    if (luaZ_sizebuffer(&g->buff) > LUA_MINBUFFER*2) {  /* buffer too big? */
	size_t newsize = luaZ_sizebuffer(&g->buff) / 2;
	luaZ_resizebuffer(L, &g->buff, newsize);
    }
}
#endif

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
}


#if 0 /* klisp: keep this around */
static void markmt (global_State *g) {
    int i;
    for (i=0; i<NUM_TAGS; i++)
	if (g->mt[i]) markobject(g, g->mt[i]);
}
#endif

/* mark root set */
static void markroot (klisp_State *K) {
    K->gray = NULL;
    K->grayagain = NULL; /* for now in klisp this isn't used */
    K->weak = NULL; /* for now in klisp this isn't used */

    /* TEMP: this is quite awfull, think of other way to do this */
    /* MAYBE: some of these could be FIXED */
    markvalue(K, K->symbol_table);
    markvalue(K, K->curr_cont);
    markvalue(K, K->next_obj);
    markvalue(K, K->next_value);
    markvalue(K, K->next_env);
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
       all valid indexes are below top */
    TValue *ptr = K->sbuf;
    for (int i = 0, top = K->stop; i < top; i++, ptr++) {
	markvalue(K, *ptr);
    }

/*    markmt(g); */
    K->gcstate = GCSpropagate;
}

static void atomic (klisp_State *K) {
    size_t udsize;  /* total size of userdata to be finalized */
    /* traverse objects caught by write barrier */
    propagateall(K);

    /* klisp: for now in klisp this isn't used */
    /* remark weak tables */
    K->gray = K->weak; 
    K->weak = NULL;
#if 0 /* keep around */
    markmt(g);  /* mark basic metatables (again) */
    propagateall(g);
#endif
    /* klisp: for now in klisp this isn't used */
    /* remark gray again */
    K->gray = K->grayagain;
    K->grayagain = NULL;
    propagateall(K);

    udsize = 0; /* to init var 'till we add user data */
#if 0 /* keep around */
    udsize = klispC_separateudata(L, 0);  /* separate userdata to be finalized */
    marktmu(g);  /* mark `preserved' userdata */
    udsize += propagateall(g);  /* remark, to propagate `preserveness' */
    cleartable(g->weak);  /* remove collected objects from weak tables */
#endif
    /* flip current white */
    K->currentwhite = cast(uint16_t, otherwhite(K));
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
	/* No need to do anything in klisp, we just kept it
	   to avoid eliminating a state in the GC */
	K->gcstate = GCSsweep;  /* end sweep-string phase */
	return 0;
    }
    case GCSsweep: {
	uint32_t old = K->totalbytes;
	K->sweepgc = sweeplist(K, K->sweepgc, GCSWEEPMAX);
	if (*K->sweepgc == NULL) {  /* nothing more to sweep? */
	    /* checkSizes(K); */ /* klisp: keep this around */
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
            /* - lim/g->gcstepmul;*/     
	    K->GCthreshold = K->totalbytes + GCSTEPSIZE; 
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
/*    klisp_assert(ttype(&o->gch) != LUA_TTABLE); */
    /* must keep invariant? */
    if (K->gcstate == GCSpropagate)
	reallymarkobject(K, v);  /* restore invariant */
    else  /* don't mind */
	makewhite(K, o);  /* mark as white just to avoid other barriers */
}

#if 0 /* keep around */
void klispC_barrierback (lua_State *L, Table *t) {
    GCObject *o = obj2gco(t);
    klisp_assert(isblack(o) && !isdead(g, o));
    klisp_assert(g->gcstate != GCSfinalize && g->gcstate != GCSpause);
    black2gray(o);  /* make table gray (again) */
    t->gclist = g->grayagain;
    g->grayagain = o;
}
#endif

/* NOTE: flags is added for klisp */
void klispC_link (klisp_State *K, GCObject *o, uint8_t tt, uint8_t flags) {
    o->gch.next = K->rootgc;
    K->rootgc = o;
    o->gch.gct = klispC_white(K);
    o->gch.tt = tt;
    o->gch.flags = tt;
    /* NOTE that o->gch.gclist doesn't need to be setted */
}

