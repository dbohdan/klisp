/*
** kground.c
** Bindings in the ground environment
** See Copyright Notice in klisp.h
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "kstate.h"
#include "kobject.h"
#include "kground.h"
#include "kenvironment.h"
#include "ksymbol.h"
#include "koperative.h"
#include "kapplicative.h"
#include "kerror.h"

#include "kghelpers.h"
#include "kgbooleans.h"
#include "kgeqp.h"
#include "kgequalp.h"
#include "kgsymbols.h"
#include "kgcontrol.h"
#include "kgpairs_lists.h"
#include "kgpair_mut.h"
#include "kgenvironments.h"
#include "kgenv_mut.h"
#include "kgcombiners.h"
#include "kgcontinuations.h"
#include "kgencapsulations.h"
#include "kgpromises.h"
#include "kgkd_vars.h"
#include "kgks_vars.h"
#include "kgnumbers.h"
#include "kgstrings.h"
#include "kgchars.h"
#include "kgports.h"

/*
** BEWARE: this is highly unhygienic, it assumes variables "symbol" and
** "value", both of type TValue. symbol will be bound to a symbol named by
** "n_" and can be referrenced in the var_args
*/
#define add_operative(K_, env_, n_, fn_, ...)	\
    { symbol = ksymbol_new(K_, n_); \
    value = make_operative(K_, fn_, __VA_ARGS__); \
    kadd_binding(K_, env_, symbol, value); }

#define add_applicative(K_, env_, n_, fn_, ...)	\
    { symbol = ksymbol_new(K_, n_); \
    value = make_applicative(K_, fn_, __VA_ARGS__); \
    kadd_binding(K_, env_, symbol, value); }

/*
** This is called once to bind all symbols in the ground environment
*/
void kinit_ground_env(klisp_State *K)
{
    TValue ground_env = K->ground_env;
    TValue symbol, value;

    /*
    ** This section will roughly follow the report and will reference the
    ** section in which each symbol is defined
    */

    /*
    **
    ** 4 Core types and primitive features
    **
    */

    /*
    ** 4.1 Booleans
    */

    /* 4.1.1 boolean? */
    add_applicative(K, ground_env, "boolean?", typep, 2, symbol, 
		    i2tv(K_TBOOLEAN));

    /*
    ** 4.2 Equivalence under mutation
    */

    /* 4.2.1 eq? */
    /* 6.5.1 eq? */
    add_applicative(K, ground_env, "eq?", eqp, 0);

    /*
    ** 4.3 Equivalence up to mutation
    */

    /* 4.3.1 equal? */
    /* 6.6.1 equal? */
    add_applicative(K, ground_env, "equal?", equalp, 0);

    /*
    ** 4.4 Symbols
    */

    /* 4.4.1 symbol? */
    add_applicative(K, ground_env, "symbol?", typep, 2, symbol, 
		    i2tv(K_TSYMBOL));

    /*
    ** 4.5 Control
    */

    /* 4.5.1 inert? */
    add_applicative(K, ground_env, "inert?", typep, 2, symbol, 
		    i2tv(K_TINERT));

    /* 4.5.2 $if */
    add_operative(K, ground_env, "$if", Sif, 0);

    /*
    ** 4.6 Pairs and lists
    */

    /* 4.6.1 pair? */
    add_applicative(K, ground_env, "pair?", typep, 2, symbol, 
		    i2tv(K_TPAIR));

    /* 4.6.2 null? */
    add_applicative(K, ground_env, "null?", typep, 2, symbol, 
		    i2tv(K_TNIL));
    
    /* 4.6.3 cons */
    add_applicative(K, ground_env, "cons", cons, 0);

    /*
    ** 4.7 Pair mutation
    */

    /* 4.7.1 set-car!, set-cdr! */
    add_applicative(K, ground_env, "set-car!", set_carB, 0);
    add_applicative(K, ground_env, "set-cdr!", set_cdrB, 0);

    /* 4.7.2 copy-es-immutable */
    add_applicative(K, ground_env, "copy-es-immutable", copy_es, 2, symbol, 
		    b2tv(false));

    /*
    ** 4.8 Environments
    */

    /* 4.8.1 environment? */
    add_applicative(K, ground_env, "environment?", typep, 2, symbol, 
		    i2tv(K_TENVIRONMENT));

    /* 4.8.2 ignore? */
    add_applicative(K, ground_env, "ignore?", typep, 2, symbol, 
		    i2tv(K_TIGNORE));

    /* 4.8.3 eval */
    add_applicative(K, ground_env, "eval", eval, 0);

    /* 4.8.4 make-environment */
    add_applicative(K, ground_env, "make-environment", make_environment, 0);

    /*
    ** 4.9 Environment mutation
    */

    /* 4.9.1 $define! */
    add_operative(K, ground_env, "$define!", SdefineB, 1, symbol);

    /*
    ** 4.10 Combiners
    */

    /* 4.10.1 operative? */
    add_applicative(K, ground_env, "operative?", typep, 2, symbol, 
		    i2tv(K_TOPERATIVE));

    /* 4.10.2 applicative? */
    add_applicative(K, ground_env, "applicative?", typep, 2, symbol, 
		    i2tv(K_TAPPLICATIVE));

    /* 4.10.3 $vau */
    /* 5.3.1 $vau */
    add_operative(K, ground_env, "$vau", Svau, 0);

    /* 4.10.4 wrap */
    add_applicative(K, ground_env, "wrap", wrap, 0);

    /* 4.10.5 unwrap */
    add_applicative(K, ground_env, "unwrap", unwrap, 0);

    /*
    **
    ** 5 Core library features (I)
    **
    */

    /*
    ** 5.1 Control
    */

    /* 5.1.1 $sequence */
    add_operative(K, ground_env, "$sequence", Ssequence, 0);

    /*
    ** 5.2 Pairs and lists
    */

    /* 5.2.1 list */
    add_applicative(K, ground_env, "list", list, 0);

    /* 5.2.2 list* */
    add_applicative(K, ground_env, "list*", listS, 0);

    /*
    ** 5.3 Combiners
    */

    /* 5.3.1 $vau */
    /* DONE: above, together with 4.10.4 */

    /* 5.3.2 $lambda */
    add_operative(K, ground_env, "$lambda", Slambda, 0);

    /*
    ** 5.4 Pairs and lists
    */

    /* 5.4.1 car, cdr */
    add_applicative(K, ground_env, "car", c_ad_r, 2, symbol, 
		    C_AD_R_PARAM(1, 0x0000));
    add_applicative(K, ground_env, "cdr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(1, 0x0001));

    /* 5.4.2 caar, cadr, ... cddddr */
    add_applicative(K, ground_env, "caar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0000));
    add_applicative(K, ground_env, "cadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0001));
    add_applicative(K, ground_env, "cdar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0010));
    add_applicative(K, ground_env, "cddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(2, 0x0011));

    add_applicative(K, ground_env, "caaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0000));
    add_applicative(K, ground_env, "caadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0001));
    add_applicative(K, ground_env, "cadar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0010));
    add_applicative(K, ground_env, "caddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0011));
    add_applicative(K, ground_env, "cdaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0100));
    add_applicative(K, ground_env, "cdadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0101));
    add_applicative(K, ground_env, "cddar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0110));
    add_applicative(K, ground_env, "cdddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(3, 0x0111));

    add_applicative(K, ground_env, "caaaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0000));
    add_applicative(K, ground_env, "caaadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0001));
    add_applicative(K, ground_env, "caadar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0010));
    add_applicative(K, ground_env, "caaddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0011));
    add_applicative(K, ground_env, "cadaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0100));
    add_applicative(K, ground_env, "cadadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0101));
    add_applicative(K, ground_env, "caddar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0110));
    add_applicative(K, ground_env, "cadddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x0111));
    add_applicative(K, ground_env, "cdaaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1000));
    add_applicative(K, ground_env, "cdaadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1001));
    add_applicative(K, ground_env, "cdadar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1010));
    add_applicative(K, ground_env, "cdaddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1011));
    add_applicative(K, ground_env, "cddaar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1100));
    add_applicative(K, ground_env, "cddadr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1101));
    add_applicative(K, ground_env, "cdddar", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1110));
    add_applicative(K, ground_env, "cddddr", c_ad_r, 2, symbol,
		    C_AD_R_PARAM(4, 0x1111));

    /*
    ** 5.5 Combiners
    */

    /* 5.5.1 apply */
    add_applicative(K, ground_env, "apply", apply, 0);

    /*
    ** 5.6 Control
    */

    /* 5.6.1 $cond */
    add_operative(K, ground_env, "$cond", Scond, 0);

    /*
    ** 5.7 Pairs and lists
    */

    /* 5.7.1 get-list-metrics */
    add_applicative(K, ground_env, "get-list-metrics", get_list_metrics, 0);

    /* 5.7.2 list-tail */
    add_applicative(K, ground_env, "list-tail", list_tail, 0);

    /*
    ** 5.8 Pair mutation
    */

    /* 5.8.1 encycle! */
    add_applicative(K, ground_env, "encycle!", encycleB, 0);

    /*
    ** 5.9 Combiners
    */

    /* 5.9.1 map */
    add_applicative(K, ground_env, "map", map, 0);

    /*
    ** 5.10 Environments
    */

    /* 5.10.1 $let */
    add_operative(K, ground_env, "$let", Slet, 1, symbol);

    /*
    **
    ** 6 Core library features (II)
    **
    */

    /*
    ** 6.1 Booleans
    */

    /* 6.1.1 not? */
    add_applicative(K, ground_env, "not?", notp, 0);

    /* 6.1.2 and? */
    add_applicative(K, ground_env, "and?", andp, 0);

    /* 6.1.3 or? */
    add_applicative(K, ground_env, "or?", orp, 0);

    /* 6.1.4 $and? */
    add_operative(K, ground_env, "$and?", Sandp_Sorp, 2, symbol, KFALSE);

    /* 6.1.5 $or? */
    add_operative(K, ground_env, "$or?", Sandp_Sorp, 2, symbol, KTRUE);

    /*
    ** 6.2 Combiners
    */

    /* 6.2.1 combiner? */
    add_applicative(K, ground_env, "combiner?", ftypep, 2, symbol, 
		    p2tv(kcombinerp));

    /*
    ** 6.3 Pairs and lists
    */

    /* 6.3.1 length */
    add_applicative(K, ground_env, "length", length, 0);

    /* 6.3.2 list-ref */
    add_applicative(K, ground_env, "list-ref", list_ref, 0);

    /* 6.3.3 append */
    add_applicative(K, ground_env, "append", append, 0);

    /* 6.3.4 list-neighbors */
    add_applicative(K, ground_env, "list-neighbors", list_neighbors, 0);

    /* 6.3.5 filter */
    add_applicative(K, ground_env, "filter", filter, 0);

    /* 6.3.6 assoc */
    add_applicative(K, ground_env, "assoc", assoc, 0);

    /* 6.3.7 member? */
    add_applicative(K, ground_env, "member?", memberp, 0);

    /* 6.3.8 finite-list? */
    add_applicative(K, ground_env, "finite-list?", finite_listp, 0);

    /* 6.3.9 countable-list? */
    add_applicative(K, ground_env, "countable-list?", countable_listp, 0);

    /* 6.3.10 reduce */
    add_applicative(K, ground_env, "reduce", reduce, 0);

    /*
    ** 6.4 Pair mutation
    */

    /* 6.4.1 append! */
    add_applicative(K, ground_env, "append!", appendB, 0);

    /* 6.4.2 copy-es */
    add_applicative(K, ground_env, "copy-es", copy_es, 2, symbol, b2tv(true));

    /* 6.4.3 assq */
    add_applicative(K, ground_env, "assq", assq, 0);

    /* 6.4.3 memq? */
    add_applicative(K, ground_env, "memq?", memqp, 0);

    /*
    ** 6.5 Equivalance under mutation
    */

    /* 6.5.1 eq? */
    /* DONE: above, together with 4.2.1 */

    /*
    ** 6.6 Equivalance up to mutation
    */

    /* 6.6.1 equal? */
    /* DONE: above, together with 4.3.1 */

    /*
    ** 6.7 Environments
    */

    /* 6.7.1 $binds? */
    add_operative(K, ground_env, "$binds?", Sbindsp, 0);

    /* 6.7.2 get-current-environment */
    add_applicative(K, ground_env, "get-current-environment", 
		    get_current_environment, 0);

    /* 6.7.3 make-kernel-standard-environment */
    add_applicative(K, ground_env, "make-kernel-standard-environment", 
		    make_kernel_standard_environment, 0);

    /* 6.7.4 $let* */
    add_operative(K, ground_env, "$let*", SletS, 1, symbol);

    /* 6.7.5 $letrec */
    add_operative(K, ground_env, "$letrec", Sletrec, 1, symbol);

    /* 6.7.6 $letrec* */
    add_operative(K, ground_env, "$letrec*", SletrecS, 1, symbol);

    /* 6.7.7 $let-redirect */
    add_operative(K, ground_env, "$let-redirect", Slet_redirect, 1, symbol);

    /* 6.7.8 $let-safe */
    add_operative(K, ground_env, "$let-safe", Slet_safe, 1, symbol);

    /* 6.7.9 $remote-eval */
    add_operative(K, ground_env, "$remote-eval", Sremote_eval, 0);

    /* 6.7.10 $bindings->environment */
    add_operative(K, ground_env, "$bindings->environment", 
		  Sbindings_to_environment, 1, symbol);

    /*
    ** 6.8 Environment mutation
    */

    /* 6.8.1 $set! */
    add_operative(K, ground_env, "$set!", SsetB, 1, symbol);

    /* 6.8.2 $provide! */
    add_operative(K, ground_env, "$provide!", SprovideB, 1, symbol);

    /* 6.8.3 $import! */
    add_operative(K, ground_env, "$import!", SimportB, 1, symbol);

    /*
    ** 6.9 Control
    */

    /* 6.9.1 for-each */
    add_applicative(K, ground_env, "for-each", for_each, 0);

    /*
    **
    ** 7 Continuations
    **
    */

    /* 
    ** 7.2 Primitive features
    */

    /* 7.1.1 continuation? */
    add_applicative(K, ground_env, "continuation?", typep, 2, symbol, 
		    i2tv(K_TCONTINUATION));

    /* 7.2.2 call/cc */
    add_applicative(K, ground_env, "call/cc", call_cc, 0);

    /* 7.2.3 extend-continuation */
    add_applicative(K, ground_env, "extend-continuation", extend_continuation, 0);

    /* 7.2.4 guard-continuation */
    add_applicative(K, ground_env, "guard-continuation", guard_continuation, 
		    0);

    /* 7.2.5 continuation->applicative */
    add_applicative(K, ground_env, "continuation->applicative",
		    continuation_applicative, 0);

    /* 7.2.6 root-continuation */
    symbol = ksymbol_new(K, "root-continuation");
    value = K->root_cont;
    kadd_binding(K, ground_env, symbol, value);
    
    /* 7.2.7 error-continuation */
    symbol = ksymbol_new(K, "error-continuation");
    value = K->error_cont;
    kadd_binding(K, ground_env, symbol, value);

    /* 
    ** 7.3 Library features
    */

    /* 7.3.1 apply-continuation */
    add_applicative(K, ground_env, "apply-continuation", apply_continuation, 
		    0);

    /* 7.3.2 $let/cc */
    add_operative(K, ground_env, "$let/cc", Slet_cc, 
		    0);

    /* 7.3.3 guard-dynamic-extent */
    add_applicative(K, ground_env, "guard-dynamic-extent", 
		    guard_dynamic_extent, 0);

    /* 7.3.4 exit */    
    add_applicative(K, ground_env, "exit", kgexit, 
		    0);


    /*
    **
    ** 8 Encapsulations
    **
    */

    /* 
    ** 8.1 Primitive features
    */

    /* 8.1.1 make-encapsulation-type */
    add_applicative(K, ground_env, "make-encapsulation-type", 
		    make_encapsulation_type, 0); 

    /*
    **
    ** 9 Promises
    **
    */

    /* 
    ** 9.1 Library features
    */

    /* 9.1.1 promise? */
    add_applicative(K, ground_env, "promise?", typep, 2, symbol, 
		    i2tv(K_TPROMISE));

    /* 9.1.2 force */
    add_applicative(K, ground_env, "force", force, 0); 

    /* 9.1.3 $lazy */
    add_operative(K, ground_env, "$lazy", Slazy, 0); 

    /* 9.1.4 memoize */
    add_applicative(K, ground_env, "memoize", memoize, 0); 

    /*
    **
    ** 10 Keyed Dynamic Variables
    **
    */

    /* 
    ** 10.1 Primitive features
    */

    /* 10.1.1 make-keyed-dynamic-variable */
    add_applicative(K, ground_env, "make-keyed-dynamic-variable", 
		    make_keyed_dynamic_variable, 0); 


    /*
    **
    ** 11 Keyed Static Variables
    **
    */

    /* 
    ** 11.1 Primitive features
    */

    /* 11.1.1 make-keyed-static-variable */
    add_applicative(K, ground_env, "make-keyed-static-variable", 
		    make_keyed_static_variable, 0); 


    /*
    **
    ** 12 Numbers
    **
    */

    /* Only integers and exact infinities for now */

    /* 
    ** 12.5 Number features
    */

    /* 12.5.1 number?, finite?, integer? */
    add_applicative(K, ground_env, "number?", ftypep, 2, symbol, 
		    p2tv(knumberp));
    add_applicative(K, ground_env, "finite?", ftyped_predp, 3, symbol, 
		    p2tv(knumberp), p2tv(kfinitep));
    add_applicative(K, ground_env, "integer?", ftypep, 2, symbol, 
		    p2tv(kintegerp));

    /* 12.5.2 =? */
    add_applicative(K, ground_env, "=?", ftyped_bpredp, 3,
		    symbol, p2tv(knumberp), p2tv(knum_eqp));
    
    /* 12.5.3 <?, <=?, >?, >=? */
    add_applicative(K, ground_env, "<?", ftyped_bpredp, 3,
		    symbol, p2tv(knumberp), p2tv(knum_ltp));
    add_applicative(K, ground_env, "<=?", ftyped_bpredp, 3,
		    symbol, p2tv(knumberp),  p2tv(knum_lep));
    add_applicative(K, ground_env, ">?", ftyped_bpredp, 3,
		    symbol, p2tv(knumberp), p2tv(knum_gtp));
    add_applicative(K, ground_env, ">=?", ftyped_bpredp, 3,
		    symbol, p2tv(knumberp), p2tv(knum_gep));

    /* 12.5.4 + */
    /* TEMP: for now only accept two arguments */
    add_applicative(K, ground_env, "+", kplus, 0);

    /* 12.5.5 * */
    /* TEMP: for now only accept two arguments */
    add_applicative(K, ground_env, "*", ktimes, 0);

    /* 12.5.6 - */
    /* TEMP: for now only accept two arguments */
    add_applicative(K, ground_env, "-", kminus, 0);

    /* 12.5.7 zero? */
    add_applicative(K, ground_env, "zero?", ftyped_predp, 3, symbol, 
		    p2tv(knumberp), p2tv(kzerop));

    /* 12.5.8 div, mod, div-and-mod */
    add_applicative(K, ground_env, "div", kdiv_mod, 2, symbol, 
		    i2tv(FDIV_DIV));
    add_applicative(K, ground_env, "mod", kdiv_mod, 2, symbol, 
		    i2tv(FDIV_MOD));
    add_applicative(K, ground_env, "div-and-mod", kdiv_mod, 2, symbol, 
		    i2tv(FDIV_DIV | FDIV_MOD));

    /* 12.5.9 div0, mod0, div0-and-mod0 */
    add_applicative(K, ground_env, "div0", kdiv_mod, 2, symbol, 
		    i2tv(FDIV_ZERO | FDIV_DIV));
    add_applicative(K, ground_env, "mod0", kdiv_mod, 2, symbol, 
		    i2tv(FDIV_ZERO | FDIV_MOD));
    add_applicative(K, ground_env, "div0-and-mod0", kdiv_mod, 2, symbol, 
		    i2tv(FDIV_ZERO | FDIV_DIV | FDIV_MOD));

    /* 12.5.10 positive?, negative? */
    add_applicative(K, ground_env, "positive?", ftyped_predp, 3, symbol, 
		    p2tv(knumberp), p2tv(kpositivep));
    add_applicative(K, ground_env, "negative?", ftyped_predp, 3, symbol, 
		    p2tv(knumberp), p2tv(knegativep));

    /* 12.5.11 odd?, even? */
    add_applicative(K, ground_env, "odd?", ftyped_predp, 3, symbol, 
		    p2tv(kintegerp), p2tv(koddp));
    add_applicative(K, ground_env, "even?", ftyped_predp, 3, symbol, 
		    p2tv(kintegerp), p2tv(kevenp));

    /* 12.5.12 abs */
    add_applicative(K, ground_env, "abs", kabs, 0);

    /* 12.5.13 min, max */
    add_applicative(K, ground_env, "min", kmin_max, 2, symbol, b2tv(FMIN));
    add_applicative(K, ground_env, "max", kmin_max, 2, symbol, b2tv(FMAX));

    /* 12.5.14 gcd, lcm */
    add_applicative(K, ground_env, "gcd", kgcd, 0);
    add_applicative(K, ground_env, "lcm", klcm, 0);

    /*
    **
    ** 13 Strings
    **
    */

    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from r5rs scheme and should not be considered standard. They are
    ** provided in the meantime to allow programs to use string features
    ** (ASCII only). 
    */

    /* 
    ** 13.1 Primitive features
    */

    /* 13.1.1? string? */
    add_applicative(K, ground_env, "string?", typep, 2, symbol, 
		    i2tv(K_TSTRING));

    /* 13.1.2? make-string */
    add_applicative(K, ground_env, "make-string", make_string, 0);

    /* 13.1.3? string-length */
    add_applicative(K, ground_env, "string-length", string_length, 0);

    /* 13.1.4? string-ref */
    add_applicative(K, ground_env, "string-ref", string_ref, 0);

    /* 13.1.5? string-set! */
    add_applicative(K, ground_env, "string-set!", string_setS, 0);

    /* 
    ** 13.2 Library features
    */

    /* 13.2.1? string */
    add_applicative(K, ground_env, "string", string, 0);

    /* 13.2.2? string=?, string-ci=? */
    add_applicative(K, ground_env, "string=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_eqp));
    add_applicative(K, ground_env, "string-ci=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_eqp));

    /* 13.2.3? string<?, string<=?, string>?, string>=? */
    add_applicative(K, ground_env, "string<?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ltp));
    add_applicative(K, ground_env, "string<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_lep));
    add_applicative(K, ground_env, "string>?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_gtp));
    add_applicative(K, ground_env, "string>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_gep));

    /* 13.2.4? string-ci<?, string-ci<=?, string-ci>?, string-ci>=? */
    add_applicative(K, ground_env, "string-ci<?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_ltp));
    add_applicative(K, ground_env, "string-ci<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_lep));
    add_applicative(K, ground_env, "string-ci>?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_gtp));
    add_applicative(K, ground_env, "string-ci>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kstringp), p2tv(kstring_ci_gep));

    /* 13.2.5? substring */
    add_applicative(K, ground_env, "substring", substring, 0);

    /* 13.2.6? string-append */
    add_applicative(K, ground_env, "string-append", string_append, 0);

    /* 13.2.7? string->list, list->string */
    add_applicative(K, ground_env, "string->list", string_to_list, 0);
    add_applicative(K, ground_env, "list->string", list_to_string, 0);

    /* 13.2.8? string-copy */
    add_applicative(K, ground_env, "string-copy", string_copy, 0);

    /* 13.2.9? string-fill! */
    add_applicative(K, ground_env, "string-fill!", string_fillS, 0);

    /*
    ** 13.3 Symbol Features (this are from section symbol in r5rs)
    */
    
    /* 13.3.1? symbol->string */
    /* TEMP: for now all strings are mutable, this returns a new object
       each time */
    add_applicative(K, ground_env, "symbol->string", symbol_to_string, 0);

    /* 13.3.2? string->symbol */
    /* TEMP: for now this can create symbols with no external representation
       this includes all symbols with non identifiers characters.
    */
    /* NOTE:
       Symbols with uppercase alphabetic characters will write as lowercase and
       so, when read again will not compare as either eq? or equal?. This is ok
       because the report only says that read objects when written and read 
       again must be equal? which happens here 
    */
    add_applicative(K, ground_env, "string->symbol", string_to_symbol, 0);
    

    /*
    **
    ** 14 Characters
    **
    */

    /*
    ** This section is still missing from the report. The bindings here are
    ** taken from r5rs scheme and should not be considered standard. They are
    ** provided in the meantime to allow programs to use character features
    ** (ASCII only). 
    */

    /* 
    ** 14.1 Primitive features
    */

    /* 14.1.1? char? */
    add_applicative(K, ground_env, "char?", typep, 2, symbol, 
		    i2tv(K_TCHAR));

    /* 14.1.2? char-alphabetic?, char-numeric?, char-whitespace? */
    /* unlike in r5rs these take an arbitrary number of chars
       (even cyclical list) */
    add_applicative(K, ground_env, "char-alphabetic?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_alphabeticp));
    add_applicative(K, ground_env, "char-numeric?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_numericp));
    add_applicative(K, ground_env, "char-whitespace?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_whitespacep));

    /* 14.1.3? char-upper-case?, char-lower-case? */
    /* unlike in r5rs these take an arbitrary number of chars
       (even cyclical list) */
    add_applicative(K, ground_env, "char-upper-case?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_upper_casep));
    add_applicative(K, ground_env, "char-lower-case?", ftyped_predp, 3, 
		    symbol, p2tv(kcharp), p2tv(kchar_lower_casep));
    

    /* 14.1.4? char->integer, integer->char */
    add_applicative(K, ground_env, "char->integer", kchar_to_integer, 0);
    add_applicative(K, ground_env, "integer->char", kinteger_to_char, 0);

    /* 14.1.4? char-upcase, char-downcase */
    add_applicative(K, ground_env, "char-upcase", kchar_upcase, 0);
    add_applicative(K, ground_env, "char-downcase", kchar_downcase, 0);

    /* 
    ** 14.2 Library features
    */

    /* 14.2.1? char=? */
    add_applicative(K, ground_env, "char=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_eqp));

    /* 14.2.2? char<?, char<=?, char>?, char>=? */
    add_applicative(K, ground_env, "char<?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ltp));
    add_applicative(K, ground_env, "char<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp),  p2tv(kchar_lep));
    add_applicative(K, ground_env, "char>?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_gtp));
    add_applicative(K, ground_env, "char>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_gep));

    /* 14.2.3? char-ci=? */
    add_applicative(K, ground_env, "char-ci=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_eqp));

    /* 14.2.4? char-ci<?, char-ci<=?, char-ci>?, char-ci>=? */
    add_applicative(K, ground_env, "char-ci<?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_ltp));
    add_applicative(K, ground_env, "char-ci<=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp),  p2tv(kchar_ci_lep));
    add_applicative(K, ground_env, "char-ci>?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_gtp));
    add_applicative(K, ground_env, "char-ci>=?", ftyped_bpredp, 3,
		    symbol, p2tv(kcharp), p2tv(kchar_ci_gep));

    /*
    **
    ** 15 Ports
    **
    */

    /* 
    ** 15.1 Primitive features
    */

    /* 15.1.1 port? */
    add_applicative(K, ground_env, "port?", typep, 2, symbol, 
		    i2tv(K_TPORT));

    /* 15.1.2 input-port?, output-port? */
    add_applicative(K, ground_env, "input-port?", ftypep, 2, symbol, 
		    p2tv(kis_input_port));

    add_applicative(K, ground_env, "output-port?", ftypep, 2, symbol, 
		    p2tv(kis_output_port));

    /* 15.1.3 with-input-from-file, with-ouput-to-file */
    add_applicative(K, ground_env, "with-input-from-file", with_file, 
		    3, symbol, b2tv(false), K->kd_in_port_key);
    add_applicative(K, ground_env, "with-output-to-file", with_file, 
		    3, symbol, b2tv(true), K->kd_out_port_key);

    /* 15.1.4 get-current-input-port, get-current-output-port */
    add_applicative(K, ground_env, "get-current-input-port", get_current_port, 
		    2, symbol, K->kd_in_port_key);
    add_applicative(K, ground_env, "get-current-output-port", get_current_port, 
		    2, symbol, K->kd_out_port_key);

    /* 15.1.5 open-input-file, open-output-file */
    add_applicative(K, ground_env, "open-input-file", open_file, 2, symbol, 
		    b2tv(false));

    add_applicative(K, ground_env, "open-output-file", open_file, 2, symbol, 
		    b2tv(true));

    /* 15.1.6 close-input-file, close-output-file */
    /* ASK John: should this be called close-input-port & close-ouput-port 
       like in r5rs? that doesn't seem consistent with open thou */
    add_applicative(K, ground_env, "close-input-file", close_file, 2, symbol, 
		    b2tv(false));

    add_applicative(K, ground_env, "close-output-file", close_file, 2, symbol, 
		    b2tv(true));

    /* 15.1.7 read */
    add_applicative(K, ground_env, "read", read, 0);

    /* 15.1.8 write */
    add_applicative(K, ground_env, "write", write, 0);

    /*
    ** These are from scheme (r5rs)
    */

    /* 15.1.? eof-object? */
    add_applicative(K, ground_env, "eof-object?", typep, 2, symbol, 
		    i2tv(K_TEOF));

    /* 15.1.? newline */
    add_applicative(K, ground_env, "newline", newline, 0);

    /* 15.1.? write-char */
    add_applicative(K, ground_env, "write-char", write_char, 0);
    
    /* 15.1.? read-char */
    add_applicative(K, ground_env, "read-char", read_peek_char, 2, symbol, 
		    b2tv(false));
    
    /* 15.1.? peek-char */
    add_applicative(K, ground_env, "peek-char", read_peek_char, 2, symbol, 
		    b2tv(true));

    /* 15.1.? char-ready? */
    /* XXX: this always return #t, proper behaviour requires platform 
       specific code (probably select for posix, a thread for windows
       (at least for files & consoles), I think pipes and sockets may
       have something */
    add_applicative(K, ground_env, "char-ready?", char_readyp, 0);

    /* 
    ** 15.2 Library features
    */

    /* 15.2.1 call-with-input-file, call-with-output-file */
    add_applicative(K, ground_env, "call-with-input-file", call_with_file, 
		    2, symbol, b2tv(false));
    add_applicative(K, ground_env, "call-with-output-file", call_with_file, 
		    2, symbol, b2tv(true));

    /* 15.2.2 load */
    add_applicative(K, ground_env, "load", load, 0);

    /* 15.2.3 get-module */
    add_applicative(K, ground_env, "get-module", get_module, 0);

    /* 15.2.? display */
    add_applicative(K, ground_env, "display", display, 0);

    /* MAYBE: That's all there is in the report combined with r5rs scheme, 
       but we will probably need: file-exists?, rename-file and remove-file.
       It would also be good to be able to select between append, truncate and
       error if a file exists, but that would need to be an option in all three 
       methods of opening. Also some directory checking, traversing etc */

    return;
}
