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
#include "kgports.h"
#include "kgchars.h"

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
    add_applicative(K, ground_env, "eq?", eqp, 0);

    /*
    ** 4.3 Equivalence up to mutation
    */

    /* 4.3.1 equal? */
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
    add_applicative(K, ground_env, "copy-es-immutable", copy_es_immutable, 
		    1, symbol);

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
    /* TODO */

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
    /* TODO */

    /*
    ** 5.10 Environments
    */

    /* 5.10.1 $let */
    /* TODO */


    /*
    **
    ** 6 Core library features (II)
    **
    */

    /* ... */

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
    /* TODO */

    /* 14.2.2? char<?, char<=?, char>?, char>=? */
    /* TODO */

    /* 14.2.3? char-ci=? */
    /* TODO */

    /* 14.2.4? char-ci<?, char-ci<=?, char-ci>?, char-ci>=? */
    /* TODO */

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
    /* TODO */

    /* 15.1.4 get-current-input-port, get-current-output-port */
    /* TODO */

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

    /* 
    ** 15.2 Library features
    */

    /* 15.2.1 call-with-input-file, call-with-output-file */
    /* TODO */

    /* 15.2.2 load */
    add_applicative(K, ground_env, "load", load, 0);

    /* 15.2.3 get-module */
    add_applicative(K, ground_env, "get-module", get_module, 0);

    /* TODO: That's all there is in the report, but we will probably need:
       (from r5rs) char-ready?, read-char, peek-char, eof-object?, newline,
       and write-char; (from c) file-exists?, rename-file and remove-file.
       It would also be good to be able to select between append, truncate and
       error if a file exists, but that would need to be an option in all three 
       methods of opening */
    

    return;

}
