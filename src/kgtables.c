/*
** kgtables.c
** Hash table interface for the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kapplicative.h"
#include "koperative.h"
#include "kcontinuation.h"
#include "kerror.h"
#include "kpair.h"

#include "kghelpers.h"
#include "kgtables.h"

/* Provide lisp interface to internal hash tables. The interface
 * is modeled after SRFI-69.
 *
 * MISSING FUNCTIONALITY
 *   - no user definable equivalence predicates
 *   - no user definable hash functions
 *   - hash function itself is not available
 *   - hash-table-ref/default not implemented
 *   - hash-table-update! not implemented
 *   - hash-table-for-each (hash-table-walk in SRFI-69) not implemented
 *
 * DEVIATIONS FROM SRFI-69
 *   - hash-table-size renamed to hash-table-length to match klisp's vector-length
 *   - hash-table-exists? and hash-table-delete! accept more than one key
 *   - hash-table-merge! accepts more than two arguments
 *
 * KNOWN BUGS
 *   - removing elements do not cause hash tables shrink
 *   - hash_table_merge() may compute too low initial table size
 *   - "array" optimization never used
 *
 * Hash tables are equal? if and only if they are eq?. Hash
 * tables do not have external representation.
 *
 * BASIC OPERATIONS
 *
 * (hash-table? OBJECT...)
 *   Type predicate. Evaluates to #t iff all arguments are hash
 *   tables, and #f otherwise.
 *
 * (make-hash-table)
 *   Create new, empty hash table. Currently accepts no optional
 *   parameters (SRFI-69 allows user-defined hash function, etc.)
 *
 * (hash-table-set! TABLE KEY VALUE)
 *   Set KEY => VALUE in TABLE, silently replacing
 *   any existing binding. The result is #inert.
 *
 * (hash-table-ref TABLE KEY [THUNK])
 *   Returns value corresponding to KEY in TABLE, if present.
 *   If KEY is not bound and THUNK is given, returns result of
 *   evaluation of (THUNK) in the dynamic environment. Otherwise,
 *   an error is signalled.
 *
 * (hash-table-exists? TABLE KEY1 KEY2 ...)
 *   Returns #t if all keys KEY1, KEY2, ... are bound in TABLE.
 *   Returns #f otherwise.
 *
 * (hash-table-delete! TABLE KEY1 KEY2 ...)
 *   Removes binding of KEY1, KEY2, ... from TABLE. If keys are not
 *   present, nothing happens. The result is #inert.
 *
 * (hash-table-length TABLE)
 *   Returns number of KEY => VALUE bindings in TABLE.
 *
 * (hash-table-copy TABLE)
 *   Returns a copy of TABLE.
 *
 * (hash-table-merge T1 T2 ... Tn)
 *   Creates new hash table with all bindings from T1, T2, ... Tn.
 *   If more than one of the tables bind the same key, only the
 *   value from the table which is comes last in the argument
 *   list is preserved.
 *
 * (hash-table-merge! DEST T1 T2 ... Tn)
 *   Copy all bindings from T1, T2, ... Tn to DEST. If more than
 *   one of the tables bind the same key, only the value from the
 *   table which is comes last in the argument list is preserved.
 *   The result is #inert.
 *
 * HIGH-LEVEL CONSTRUCTORS
 *
 * (hash-table K1 V1 K2 V2 ...)
 *   Creates new hash table, binding Kn => Vn. If Ki = Kj for i < j,
 *   then Vj overrides Vi.
 *
 * (alist->hash-table ALIST)
 *   Creates new hash table from association list.
 *
 * WHOLE CONTENTS MANIPULATION
 *
 * (hash-table->alist TABLE)
 *   Returns list (KEY . VALUE) pairs from TABLE.
 *
 * (hash-table-keys TABLE)
 *   Returns list of all keys from TABLE.
 *
 * (hash-table-values TABLE)
 *   Returns list of all values from TABLE.
 *
 */

static void make_hash_table(klisp_State *K)
{
    check_0p(K, K->next_value);
    TValue tab = klispH_new(K,
                            0,  /* narray - not used in klisp */
                            32, /* nhash - size of the hash table */
                            0   /* wflags - no weak pointers */ );
    kapply_cc(K, tab);
}

static void hash_table_setB(klisp_State *K)
{
    bind_3tp(K, K->next_value,
             "hash table", ttistable, tab,
             "any", anytype, key,
             "any", anytype, val);
    *klispH_set(K, tv2table(tab), key) = val;
    kapply_cc(K, KINERT);
}

static void hash_table_ref(klisp_State *K)
{
    bind_al2tp(K, K->next_value,
               "hash table", ttistable, tab,
               "any", anytype, key,
               dfl);
    (void) get_opt_tpar(K, dfl, "combiner", ttiscombiner);

    const TValue *node = klispH_get(tv2table(tab), key);
    if (!ttisfree(*node)) {
        kapply_cc(K, *node);
    } else if (ttiscombiner(dfl)) {
        while(ttisapplicative(dfl))
            dfl = tv2app(dfl)->underlying;
        ktail_call(K, dfl, KNIL, K->next_env);
    } else {
        klispE_throw_simple_with_irritants(K, "key not found",
                                           1, key);
    }
}

static void hash_table_existsP(klisp_State *K)
{
    int32_t i, pairs;
    TValue res = KTRUE;
    bind_al1tp(K, K->next_value,
               "hash table", ttistable, tab,
               keys);
    check_list(K, 1, keys, &pairs, NULL);

    for (i = 0; i < pairs; i++, keys = kcdr(keys)) {
        const TValue *node = klispH_get(tv2table(tab), kcar(keys));
        if (ttisfree(*node)) {
            res = KFALSE;
            break;
        }
    }
    kapply_cc(K, res);
}

static void hash_table_deleteB(klisp_State *K)
{
    int32_t i, pairs;
    bind_al1tp(K, K->next_value,
               "hash table", ttistable, tab,
               keys);
    check_list(K, 1, keys, &pairs, NULL);

    for (i = 0; i < pairs; i++, keys = kcdr(keys)) {
        TValue *node = klispH_set(K, tv2table(tab), kcar(keys));
        if (!ttisfree(*node)) {
            *node = KFREE; /* TODO: shrink ? */
        }
    }
    kapply_cc(K, KINERT);
}

static void hash_table_length(klisp_State *K)
{
    bind_1tp(K, K->next_value, "hash table", ttistable, tab);
    kapply_cc(K, i2tv(klispH_numuse(tv2table(tab))));
}

static void hash_table_constructor(klisp_State *K)
{
    int32_t pairs, cpairs, i;
    TValue rest = K->next_value;
    check_list(K, 1, rest, &pairs, &cpairs);
    if ((pairs % 2 != 0) || (cpairs % 2 != 0))
        klispE_throw_simple(K, "expected even number of arguments");

    TValue tab = klispH_new(K, 0, 32 + 2 * pairs, 0);
    krooted_tvs_push(K, tab);
    for (i = 0; i < pairs; i += 2, rest = kcddr(rest))
        *klispH_set(K, tv2table(tab), kcar(rest)) = kcadr(rest);
    krooted_tvs_pop(K);
    kapply_cc(K, tab);
}

static void alist_to_hash_table(klisp_State *K)
{
    int32_t pairs, i;
    bind_1p(K, K->next_value, rest);
    check_typed_list(K, kpairp, true, rest, &pairs, NULL);

    TValue tab = klispH_new(K, 0, 32 + 2 * pairs, 0);
    krooted_tvs_push(K, tab);
    for (i = 0; i < pairs; i++, rest = kcdr(rest))
        *klispH_set(K, tv2table(tab), kcaar(rest)) = kcdar(rest);
    krooted_tvs_pop(K);
    kapply_cc(K, tab);
}

static void hash_table_merge(klisp_State *K)
{
    int32_t pairs;
    bool destructive = bvalue(K->next_xparams[0]);
    bool only_one_arg = ivalue(K->next_xparams[1]);
    TValue dest, rest = K->next_value;

    check_typed_list(K, ktablep, true, rest, &pairs, NULL);
    if (only_one_arg && pairs != 1) {
        klispE_throw_simple(K, "expected one argument");
    }
    if (destructive) {
        if (pairs == 0)
            klispE_throw_simple(K, "expected at least one argument");
        dest = kcar(rest);
        rest = kcdr(rest);
        pairs--;
    } else {
        dest = klispH_new(K, 0, 32 + 2 * pairs, 0);
    }

    krooted_tvs_push(K, dest);
    while (pairs--) {
        TValue key = KFREE, data;
        Table *t = tv2table(kcar(rest));
        while (klispH_next(K, t, &key, &data))
            *klispH_set(K, tv2table(dest), key) = data;
        rest = kcdr(rest);
    }
    krooted_tvs_pop(K);

    kapply_cc(K, (destructive ? KINERT : dest));
}

/* table_elements(K, TAB, MKELT) calls MKELT(key, value)
 * on each key=>value binding in TAB and returns a list
 * of objects returned by MKELT. TAB must be rooted.
 */
static TValue table_elements
    (klisp_State *K, Table *t,
     TValue (*mkelt)(klisp_State *K, TValue k, TValue v))
{
    TValue key = KFREE, data, res = KNIL, elt = KINERT;

    krooted_vars_push(K, &res);
    krooted_vars_push(K, &elt);
    while (klispH_next(K, t, &key, &data)) {
        elt = mkelt(K, key, data);
        res = kcons(K, elt, res);
    }
    krooted_vars_pop(K);
    krooted_vars_pop(K);
    return res;
}

static TValue mkelt_proj1(klisp_State *K, TValue k, TValue v)
{
    UNUSED(K);
    UNUSED(v);
    return k;
}

static TValue mkelt_proj2(klisp_State *K, TValue k, TValue v)
{
    UNUSED(K);
    UNUSED(k);
    return v;
}

static TValue mkelt_cons(klisp_State *K, TValue k, TValue v)
{
    return kcons(K, k, v);
}

static void hash_table_to_list(klisp_State *K)
{
    bind_1tp(K, K->next_value, "hash table", ttistable, tab);
    TValue res = table_elements(K, tv2table(tab), pvalue(K->next_xparams[0]));
    kapply_cc(K, res);
}

/* init ground */
void kinit_tables_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    add_applicative(K, ground_env, "hash-table?", typep, 2, symbol,
                    i2tv(K_TTABLE));
    add_applicative(K, ground_env, "make-hash-table", make_hash_table, 0);

    add_applicative(K, ground_env, "hash-table-set!", hash_table_setB, 0);
    add_applicative(K, ground_env, "hash-table-ref", hash_table_ref, 0);
    add_applicative(K, ground_env, "hash-table-exists?", hash_table_existsP, 0);
    add_applicative(K, ground_env, "hash-table-delete!", hash_table_deleteB, 0);
    add_applicative(K, ground_env, "hash-table-length", hash_table_length, 0);

    add_applicative(K, ground_env, "hash-table", hash_table_constructor, 0);
    add_applicative(K, ground_env, "alist->hash-table", alist_to_hash_table, 0);

    add_applicative(K, ground_env, "hash-table-merge", hash_table_merge, 2, KFALSE, KFALSE);
    add_applicative(K, ground_env, "hash-table-copy", hash_table_merge, 2, KFALSE, KTRUE);
    add_applicative(K, ground_env, "hash-table-merge!", hash_table_merge, 2, KTRUE, KFALSE);

    add_applicative(K, ground_env, "hash-table-keys", hash_table_to_list, 1, p2tv(mkelt_proj1));
    add_applicative(K, ground_env, "hash-table-values", hash_table_to_list, 1, p2tv(mkelt_proj2));
    add_applicative(K, ground_env, "hash-table->alist", hash_table_to_list, 1, p2tv(mkelt_cons));
}
