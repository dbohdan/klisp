/*
** imrat.h
** Arbitrary precision rational arithmetic routines.
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: This is mostly from the IMath library, written by
** M.J. Fromberger. It is adapted to klisp, mainly in the use of the
** klisp allocator and fixing of digit size to 32 bits.
** Imported from version (1.15) updated 01-Feb-2011 at 03:10 PM.
*/

#ifndef IMRAT_H_
#define IMRAT_H_

#include "imath.h"

/* Andres Navarro: klisp includes */
#include "kobject.h"
#include "kstate.h"

#ifdef USE_C99
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Andres Navarro: Use kobject type instead */
typedef Bigrat mpq_t, *mp_rat;

#if 0
typedef struct mpq {
  mpz_t   num;    /* Numerator         */
  mpz_t   den;    /* Denominator, <> 0 */
} mpq_t, *mp_rat;
#endif

#define MP_NUMER_P(Q)  (&((Q)->num)) /* Pointer to numerator   */
#define MP_DENOM_P(Q)  (&((Q)->den)) /* Pointer to denominator */

/* Rounding constants */
typedef enum { 
  MP_ROUND_DOWN, 
  MP_ROUND_HALF_UP, 
  MP_ROUND_UP, 
  MP_ROUND_HALF_DOWN 
} mp_round_mode;

mp_result mp_rat_init(klisp_State *K, mp_rat r);
mp_rat    mp_rat_alloc(klisp_State *K);
mp_result mp_rat_init_size(klisp_State *K, mp_rat r, mp_size n_prec, 
			   mp_size d_prec);
mp_result mp_rat_init_copy(klisp_State *K, mp_rat r, mp_rat old);
mp_result mp_rat_set_value(klisp_State *K, mp_rat r, int numer, int denom);
void      mp_rat_clear(klisp_State *K, mp_rat r);
void      mp_rat_free(klisp_State *K, mp_rat r);
mp_result mp_rat_numer(klisp_State *K, mp_rat r, mp_int z);  /* z = num(r)  */
mp_result mp_rat_denom(klisp_State *K, mp_rat r, mp_int z);  /* z = den(r)  */
/* NOTE: this doesn't use the allocator */
mp_sign   mp_rat_sign(mp_rat r);

mp_result mp_rat_copy(klisp_State *K, mp_rat a, mp_rat c);  /* c = a       */
/* NOTE: this doesn't use the allocator */
void      mp_rat_zero(mp_rat r);                        /* r = 0       */
mp_result mp_rat_abs(klisp_State *K, mp_rat a, mp_rat c); /* c = |a|     */
mp_result mp_rat_neg(klisp_State *K, mp_rat a, mp_rat c); /* c = -a      */
mp_result mp_rat_recip(klisp_State *K, mp_rat a, mp_rat c); /* c = 1 / a   */
/* c = a + b   */
mp_result mp_rat_add(klisp_State *K, mp_rat a, mp_rat b, mp_rat c);
/* c = a - b   */
mp_result mp_rat_sub(klisp_State *K, mp_rat a, mp_rat b, mp_rat c);
/* c = a * b   */
mp_result mp_rat_mul(klisp_State *K, mp_rat a, mp_rat b, mp_rat c);
/* c = a / b   */
mp_result mp_rat_div(klisp_State *K, mp_rat a, mp_rat b, mp_rat c);

/* c = a + b   */
mp_result mp_rat_add_int(klisp_State *K, mp_rat a, mp_int b, mp_rat c);
/* c = a - b   */
mp_result mp_rat_sub_int(klisp_State *K, mp_rat a, mp_int b, mp_rat c); 
/* c = a * b   */
mp_result mp_rat_mul_int(klisp_State *K, mp_rat a, mp_int b, mp_rat c); 
/* c = a / b   */
mp_result mp_rat_div_int(klisp_State *K, mp_rat a, mp_int b, mp_rat c); 
/* c = a ^ b   */
mp_result mp_rat_expt(klisp_State *K, mp_rat a, mp_small b, mp_rat c);  

/* NOTE: because we may need to do multiplications, some of 
   these take a klisp_State */
int       mp_rat_compare(klisp_State *K, mp_rat a, mp_rat b); /* a <=> b */
/* |a| <=> |b| */
int       mp_rat_compare_unsigned(klisp_State *K, mp_rat a, mp_rat b);  
int       mp_rat_compare_zero(mp_rat r); /* r <=> 0     */
int       mp_rat_compare_value(klisp_State *K, mp_rat r, mp_small n, 
			       mp_small d); /* r <=> n/d */
int       mp_rat_is_integer(mp_rat r);

/* Convert to integers, if representable (returns MP_RANGE if not). */
/* NOTE: this doesn't use the allocator */
mp_result mp_rat_to_ints(mp_rat r, mp_small *num, mp_small *den);

/* Convert to nul-terminated string with the specified radix, writing
   at most limit characters including the nul terminator. */
mp_result mp_rat_to_string(mp_rat r, mp_size radix, char *str, int limit);

/* Convert to decimal format in the specified radix and precision,
   writing at most limit characters including a nul terminator. */
mp_result mp_rat_to_decimal(mp_rat r, mp_size radix, mp_size prec,
                            mp_round_mode round, char *str, int limit);

/* Return the number of characters required to represent r in the given
   radix.  May over-estimate. */
mp_result mp_rat_string_len(mp_rat r, mp_size radix);

/* Return the number of characters required to represent r in decimal
   format with the given radix and precision.  May over-estimate. */
mp_result mp_rat_decimal_len(mp_rat r, mp_size radix, mp_size prec);

/* Read zero-terminated string into r */
mp_result mp_rat_read_string(klisp_State *K, mp_rat r, mp_size radix, 
			     const char *str);
mp_result mp_rat_read_cstring(klisp_State *K, mp_rat r, mp_size radix, 
			      const char *str, char **end);
mp_result mp_rat_read_ustring(klisp_State *K, mp_rat r, mp_size radix, 
			      const char *str, char **end);

/* Read zero-terminated string in decimal format into r */
mp_result mp_rat_read_decimal(klisp_State *K, mp_rat r, mp_size radix, 
			      const char *str);
mp_result mp_rat_read_cdecimal(klisp_State *K, mp_rat r, mp_size radix, 
			       const char *str, char **end);

#ifdef __cplusplus
}
#endif
#endif /* IMRAT_H_ */
