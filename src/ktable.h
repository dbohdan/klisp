/*
** ktable.h
** Kernel Hashtables
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: This is almost textually from lua.
** Parts that don't apply, or don't apply yet to klisp are in comments.
*/

#ifndef ktable_h
#define ktable_h

#include "kobject.h"
#include "kstate.h"

#define gnode(t,i)	(&(t)->node[i])
#define gkey(n)		(&(n)->i_key.nk)
#define gval(n)		((n)->i_val)
#define gnext(n)	((n)->i_key.nk.next)

#define key2tval(n)	((n)->i_key.tvk)

const TValue *klispH_getfixint (Table *t, int32_t key);
TValue *klispH_setfixint (klisp_State *K, Table *t, int32_t key);
const TValue *klispH_getstr (Table *t, String *key);
TValue *klispH_setstr (klisp_State *K, Table *t, String *key);
const TValue *klispH_get (Table *t, TValue key);
TValue *klispH_set (klisp_State *K, Table *t, TValue key);
TValue klispH_new (klisp_State *K, int32_t narray, int32_t lnhash);
void klispH_resizearray (klisp_State *K, Table *t, int32_t nasize);
void klispH_free (klisp_State *K, Table *t);
#if 0 /* not for now */
int32_t klispH_next (klisp_State *K, Table *t, StkId key);
int32_t klispH_getn (Table *t);
#endif

#endif
