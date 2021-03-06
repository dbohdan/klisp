/*
** krational.h
** Kernel Rationals (fixrats and bigrats)
** See Copyright Notice in klisp.h
*/

#ifndef krational_h
#define krational_h

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

#include "kobject.h"
#include "kstate.h"
#include "kinteger.h"
#include "imrat.h"

/* TEMP: for now we only implement bigrats (memory allocated) */

/* This tries to convert a bigrat to a fixint or a bigint */
static inline TValue kbigrat_try_integer(klisp_State *K, TValue n)
{
    Bigrat *b = tv2bigrat(n);

    if (!mp_rat_is_integer(b))
        return n;

    /* sadly we have to repeat the code from try_fixint here... */
    Bigint *i = MP_NUMER_P(b);
    if (MP_USED(i) == 1) {
        int64_t digit = (int64_t) *(MP_DIGITS(i));
        if (MP_SIGN(i) == MP_NEG) digit = -digit;
        if (kfit_int32_t(digit))
            return i2tv((int32_t) digit); 
        /* else fall through */
    }
    /* should alloc a bigint */
    /* GC: n may not be rooted */
    krooted_tvs_push(K, n);
    TValue copy = kbigint_copy(K, gc2bigint(i));
    krooted_tvs_pop(K);
    return copy;
}

/* used in reading and for res & temps in operations */
TValue kbigrat_new(klisp_State *K, bool sign, uint32_t num, 
                   uint32_t den);

/* used in write to destructively get the digits */
TValue kbigrat_copy(klisp_State *K, TValue src);

/* macro to create the simplest rational */
#define kbigrat_make_simple(K_) kbigrat_new(K_, false, 0, 1)

/* Create a stack allocated bigrat from a bigint,
   useful for mixed operations, relatively light weight compared
   to creating it in the heap and burdening the gc */
#define kbind_bigrat_fixint(name, fixint)                       \
    int32_t (KUNIQUE_NAME(i)) = ivalue(fixint);                 \
    Bigrat KUNIQUE_NAME(bigrat_i);                              \
    /* can't use unique_name bigrat because it conflicts */		\
    /* numer is 1 */                                            \
    (KUNIQUE_NAME(bigrat_i)).num.single = ({                    \
            int64_t temp = (KUNIQUE_NAME(i));                   \
            (uint32_t) ((temp < 0)? -temp : temp);              \
        });                                                     \
    (KUNIQUE_NAME(bigrat_i)).num.digits =                       \
        &((KUNIQUE_NAME(bigrat_i)).num.single);                 \
    (KUNIQUE_NAME(bigrat_i)).num.alloc = 1;                     \
    (KUNIQUE_NAME(bigrat_i)).num.used = 1;                      \
    (KUNIQUE_NAME(bigrat_i)).num.sign = (KUNIQUE_NAME(i)) < 0?  \
        MP_NEG : MP_ZPOS;                                       \
    /* denom is 1 */                                            \
    (KUNIQUE_NAME(bigrat_i)).den.single = 1;                    \
    (KUNIQUE_NAME(bigrat_i)).den.digits =                       \
        &((KUNIQUE_NAME(bigrat_i)).den.single);                 \
    (KUNIQUE_NAME(bigrat_i)).den.alloc = 1;                     \
    (KUNIQUE_NAME(bigrat_i)).den.used = 1;                      \
    (KUNIQUE_NAME(bigrat_i)).den.sign = MP_ZPOS;                \
                                                                \
    Bigrat *name = &(KUNIQUE_NAME(bigrat_i))

#define kbind_bigrat_bigint(name, bigint)                               \
    Bigint *KUNIQUE_NAME(bi) = tv2bigint(bigint);                       \
                             Bigrat KUNIQUE_NAME(bigrat);               \
                             /* numer is bigint */						\
                             (KUNIQUE_NAME(bigrat)).num.single = (KUNIQUE_NAME(bi))->single; \
                             (KUNIQUE_NAME(bigrat)).num.digits = (KUNIQUE_NAME(bi))->digits; \
                             (KUNIQUE_NAME(bigrat)).num.alloc = (KUNIQUE_NAME(bi))->alloc; \
                             (KUNIQUE_NAME(bigrat)).num.used = (KUNIQUE_NAME(bi))->used; \
                             (KUNIQUE_NAME(bigrat)).num.sign = (KUNIQUE_NAME(bi))->sign; \
                             /* denom is 1 */							\
                             (KUNIQUE_NAME(bigrat)).den.single = 1;     \
                             (KUNIQUE_NAME(bigrat)).den.digits =        \
        &((KUNIQUE_NAME(bigrat)).den.single);                           \
                             (KUNIQUE_NAME(bigrat)).den.alloc = 1;      \
                             (KUNIQUE_NAME(bigrat)).den.used = 1;       \
                             (KUNIQUE_NAME(bigrat)).den.sign = MP_ZPOS; \
                             Bigrat *name = &(KUNIQUE_NAME(bigrat))
    
/* XXX: Now that I think about it this (and kensure_bigint) could be more 
   cleanly implemented as a function that takes a pointer... (derp derp) */

/* This can be used prior to calling a bigrat functions
   to automatically convert fixints & bigints to bigrats.
   NOTE: calls to this macro should go in different lines! 
   and on different lines to calls to kensure_bigint */
#define kensure_bigrat(n)                                               \
    /* must use goto, no block should be entered before calling         \
       kbind_bigrat */                                                  \
    if (ttisbigrat(n))                                                  \
        goto KUNIQUE_NAME(bigrat_exit_lbl);                             \
    if (ttisbigint(n))                                                  \
        goto KUNIQUE_NAME(bigrat_bigint_lbl);                           \
    /* else ttisfixint(n) */                                            \
    kbind_bigrat_fixint(KUNIQUE_NAME(brat_i), (n));                     \
    (n) = gc2bigrat(KUNIQUE_NAME(brat_i));                              \
    goto KUNIQUE_NAME(bigrat_exit_lbl);                                 \
KUNIQUE_NAME(bigrat_bigint_lbl):                                        \
                               ; /* gcc asks for a statement (not a decl) after label */ \
                               kbind_bigrat_bigint(KUNIQUE_NAME(brat), (n)); \
                               (n) = gc2bigrat(KUNIQUE_NAME(brat));     \
KUNIQUE_NAME(bigrat_exit_lbl):

/*
** read/write interface 
*/
/* this works for bigrats, bigints & fixints, returns true if ok */
/* NOTE: doesn't allow decimal */
bool krational_read(klisp_State *K, char *buf, int32_t base, TValue *out, 
                    char **end);
/* NOTE: allow decimal for use after #e */
bool krational_read_decimal(klisp_State *K, char *buf, int32_t base, TValue *out, 
                            char **end, bool *out_decimalp);

int32_t kbigrat_print_size(TValue tv_bigrat, int32_t base);
void  kbigrat_print_string(klisp_State *K, TValue tv_bigrat, int32_t base, 
                           char *buf, int32_t limit);

/* Interface for kgnumbers */
bool kbigrat_eqp(klisp_State *K, TValue bigrat1, TValue bigrat2);

bool kbigrat_ltp(klisp_State *K, TValue bigrat1, TValue bigrat2);
bool kbigrat_lep(klisp_State *K, TValue bigrat1, TValue bigrat2);
bool kbigrat_gtp(klisp_State *K, TValue bigrat1, TValue bigrat2);
bool kbigrat_gep(klisp_State *K, TValue bigrat1, TValue bigrat2);

TValue kbigrat_plus(klisp_State *K, TValue n1, TValue n2);
TValue kbigrat_times(klisp_State *K, TValue n1, TValue n2);
TValue kbigrat_minus(klisp_State *K, TValue n1, TValue n2);
TValue kbigrat_divided(klisp_State *K, TValue n1, TValue n2);

TValue kbigrat_div_mod(klisp_State *K, TValue n1, TValue n2, TValue *res_r);
TValue kbigrat_div0_mod0(klisp_State *K, TValue n1, TValue n2, TValue *res_r);

bool kbigrat_negativep(TValue tv_bigrat);
bool kbigrat_positivep(TValue tv_bigrat);

/* needs the state to create a copy if negative */
TValue kbigrat_abs(klisp_State *K, TValue tv_bigrat);

TValue kbigrat_numerator(klisp_State *K, TValue tv_bigrat);
TValue kbigrat_denominator(klisp_State *K, TValue tv_bigrat);

typedef enum { K_FLOOR, K_CEILING, K_TRUNCATE, K_ROUND_EVEN } kround_mode;
TValue kbigrat_to_integer(klisp_State *K, TValue tv_bigrat, kround_mode mode);

TValue kbigrat_simplest_rational(klisp_State *K, TValue tv_n1, TValue tv_n2);
TValue kbigrat_rationalize(klisp_State *K, TValue tv_n1, TValue tv_n2);

#endif
