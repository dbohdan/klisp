/*
** kgc.h
** Garbage Collector
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: This is almost textually from lua.
** Parts that don't apply, or don't apply yet to klisp are in comments.
*/

#ifndef kgc_h
#define kgc_h

#include "kobject.h"
#include "kstate.h"

/*
** Possible states of the Garbage Collector
*/
#define GCSpause	0
#define GCSpropagate	1
#define GCSsweepstring	2
#define GCSsweep	3
#define GCSfinalize	4

/* NOTE: unlike in lua the gc flags have 16 bits in klisp,
   so resetbits is slightly different */

/*
** some useful bit tricks
*/
#define resetbits(x,m)	((x) &= cast(uint16_t, ~(m)))
#define setbits(x,m)	((x) |= (m))
#define testbits(x,m)	((x) & (m))
#define bitmask(b)	(1<<(b))
#define bit2mask(b1,b2)	(bitmask(b1) | bitmask(b2))
#define k_setbit(x,b)	setbits(x, bitmask(b))
#define resetbit(x,b)	resetbits(x, bitmask(b))
#define testbit(x,b)	testbits(x, bitmask(b))
#define set2bits(x,b1,b2)	setbits(x, (bit2mask(b1, b2)))
#define reset2bits(x,b1,b2)	resetbits(x, (bit2mask(b1, b2)))
#define test2bits(x,b1,b2)	testbits(x, (bit2mask(b1, b2)))

/* NOTE: in klisp there is still no userdata, threads or finalization. 
   Also the field is called gct instead of marked */

/*
** Layout for bit use in `gct' field:
** bit 0 - object is white (type 0)
** bit 1 - object is white (type 1)
** bit 2 - object is black
** bit 3 - for userdata: has been finalized
** bit 3 - for tables: has weak keys
** bit 4 - for tables: has weak values
** bit 5 - object is fixed (should not be collected)
** bit 6 - object is "super" fixed (only the main thread)
*/


#define WHITE0BIT	0
#define WHITE1BIT	1
#define BLACKBIT	2
#define FINALIZEDBIT	3
#define KEYWEAKBIT	3
#define VALUEWEAKBIT	4
#define FIXEDBIT	5
#define SFIXEDBIT	6
#define WHITEBITS	bit2mask(WHITE0BIT, WHITE1BIT)


#define iswhite(x)         test2bits((x)->gch.gct, WHITE0BIT, WHITE1BIT)
#define isblack(x)         testbit((x)->gch.gct, BLACKBIT)
#define isgray(x)	(!isblack(x) && !iswhite(x))

#define otherwhite(g)	(g->currentwhite ^ WHITEBITS)
#define isdead(g,v)	((v)->gch.gct & otherwhite(g) & WHITEBITS)

#define changewhite(x)	((x)->gch.gct ^= WHITEBITS)
#define gray2black(x)	k_setbit((x)->gch.gct, BLACKBIT)

#define valiswhite(x)	(iscollectable(x) && iswhite(gcvalue(x)))

#define klispC_white(g)	cast(uint16_t, (g)->currentwhite & WHITEBITS)


#define klispC_checkGC(K) {                     \
        if (G(K)->totalbytes >= G(K)->GCthreshold)  \
            klispC_step(K); }


#define klispC_barrier(K,p,v) { if (valiswhite(v) && isblack(obj2gco(p))) \
            klispC_barrierf(K,obj2gco(p),gcvalue(v)); }

#define klispC_barriert(K,t,v) { if (valiswhite(v) && isblack(obj2gco(t))) \
            klispC_barrierback(K,t); }

#define klispC_objbarrier(K,p,o)                        \
	{ if (iswhite(obj2gco(o)) && isblack(obj2gco(p)))   \
            klispC_barrierf(K,obj2gco(p),obj2gco(o)); }

#define klispC_objbarriert(K,t,o)                                       \
    { if (iswhite(obj2gco(o)) && isblack(obj2gco(t))) klispC_barrierback(K,t); }

/* size_t klispC_separateudata (klisp_State *K, int all); */
/* void klispC_callGCTM (klisp_State *K); */
void klispC_freeall (klisp_State *K);
void klispC_step (klisp_State *K);
void klispC_fullgc (klisp_State *K);
void klispC_link (klisp_State *K, GCObject *o, uint8_t tt, uint8_t flags);
void klispC_barrierf (klisp_State *K, GCObject *o, GCObject *v);
void klispC_barrierback (klisp_State *K, Table *t);

#endif
