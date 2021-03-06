/*
** imath.h
** Arbitrary precision integer arithmetic routines.
** See Copyright Notice in klisp.h
*/

/*
** SOURCE NOTE: This is mostly from the IMath library, written by
** M.J. Fromberger. It is adapted to klisp, mainly in the use of the
** klisp allocator and fixing of digit size to 32 bits.
** Imported from version (1.15) updated 01-Feb-2011 at 03:10 PM.
*/

#ifndef IMATH_H_
#define IMATH_H_

#include <limits.h>

/* Andres Navarro: use c99 constants */
#ifndef USE_C99
#define USE_C99 1
#endif

/* Andres Navarro: klisp includes */
#include "kobject.h"
#include "kstate.h"

#ifdef USE_C99
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if USE_C99
    typedef unsigned char         mp_sign;
    typedef uint32_t                 mp_size;
    typedef int                         mp_result;
    typedef int32_t                  mp_small;  /* must be a signed type */
    typedef uint32_t                 mp_usmall; /* must be an unsigned type */
    typedef uint32_t                 mp_digit;
    typedef uint64_t                 mp_word;
#else /* USE_C99 */
    typedef unsigned char         mp_sign;
    typedef unsigned int          mp_size;
    typedef int                         mp_result;
    typedef long                        mp_small;  /* must be a signed type */
    typedef unsigned long         mp_usmall; /* must be an unsigned type */
#ifdef USE_LONG_LONG
    typedef unsigned int          mp_digit;
    typedef unsigned long long mp_word;
#else /* USE_LONG_LONG */
    typedef unsigned short        mp_digit;
    typedef unsigned int          mp_word;
#endif /* USE_LONG_LONG */
#endif /* USE_C99 */

/* Andres Navarro: Use kobject type instead */
    typedef Bigint mpz_t, *mp_int;

#if 0
    typedef struct mpz {
        mp_digit    single;
        mp_digit   *digits;
        mp_size        alloc;
        mp_size        used;
        mp_sign        sign;
    } mpz_t, *mp_int;
#endif

#define MP_SINGLE(Z) ((Z)->single) /* added to correct check in mp_int_clear */
#define MP_DIGITS(Z) ((Z)->digits)
#define MP_ALLOC(Z)  ((Z)->alloc)
#define MP_USED(Z)   ((Z)->used)
#define MP_SIGN(Z)   ((Z)->sign)

    extern const mp_result MP_OK;
    extern const mp_result MP_FALSE;
    extern const mp_result MP_TRUE;
    extern const mp_result MP_MEMORY;
    extern const mp_result MP_RANGE;
    extern const mp_result MP_UNDEF;
    extern const mp_result MP_TRUNC;
    extern const mp_result MP_BADARG;
    extern const mp_result MP_MINERR;

#define MP_DIGIT_BIT    (sizeof(mp_digit) * CHAR_BIT)
#define MP_WORD_BIT        (sizeof(mp_word) * CHAR_BIT)
/* Andres Navarro: USE_C99 */
#ifdef USE_C99
#define MP_SMALL_MIN    INT32_MIN
#define MP_SMALL_MAX    INT32_MAX
#define MP_USMALL_MIN   UINT32_MIN
#define MP_USMALL_MAX   UINT32_MAX
#define MP_DIGIT_MAX   ((uint64_t) UINT32_MAX)
#define MP_WORD_MAX    UINT64_MAX
#else /* USE_C99 */
#define MP_SMALL_MIN    LONG_MIN
#define MP_SMALL_MAX    LONG_MAX
#define MP_USMALL_MIN   ULONG_MIN
#define MP_USMALL_MAX   ULONG_MAX
#ifdef USE_LONG_LONG
#  ifndef ULONG_LONG_MAX
#    ifdef ULLONG_MAX
#         define ULONG_LONG_MAX   ULLONG_MAX
#    else
#         error "Maximum value of unsigned long long not defined!"
#    endif
#  endif
#  define MP_DIGIT_MAX   (UINT_MAX * 1ULL)
#  define MP_WORD_MAX    ULONG_LONG_MAX
#else /* USE_LONG_LONG */
#  define MP_DIGIT_MAX    (USHRT_MAX * 1UL)
#  define MP_WORD_MAX        (UINT_MAX * 1UL)
#endif /* USE_LONG_LONG */
#endif /* USE_C99 */

#define MP_MIN_RADIX    2
#define MP_MAX_RADIX    36

/* Values with fewer than this many significant digits use the
   standard multiplication algorithm; otherwise, a recursive algorithm
   is used.  Choose a value to suit your platform.  
*/
#define MP_MULT_THRESH  22

#define MP_DEFAULT_PREC 8   /* default memory allocation, in digits */

    extern const mp_sign   MP_NEG;
    extern const mp_sign   MP_ZPOS;

#define mp_int_is_odd(Z)  ((Z)->digits[0] & 1)
#define mp_int_is_even(Z) !((Z)->digits[0] & 1)

/* NOTE: this doesn't use the allocator */
    mp_result mp_int_init(mp_int z);
    mp_int    mp_int_alloc(klisp_State *K);
    mp_result mp_int_init_size(klisp_State *K, mp_int z, mp_size prec);
    mp_result mp_int_init_copy(klisp_State *K, mp_int z, mp_int old);
    mp_result mp_int_init_value(klisp_State *K, mp_int z, mp_small value);
    mp_result mp_int_set_value(klisp_State *K, mp_int z, mp_small value);
    void         mp_int_clear(klisp_State *K, mp_int z);
    void         mp_int_free(klisp_State *K, mp_int z);

    mp_result mp_int_copy(klisp_State *K, mp_int a, mp_int c); /* c = a */
/* NOTE: this doesn't use the allocator */
    void         mp_int_swap(mp_int a, mp_int c);                 /* swap a, c */
/* NOTE: this doesn't use the allocator */
    void         mp_int_zero(mp_int z);        /* z = 0 */
    mp_result mp_int_abs(klisp_State *K, mp_int a, mp_int c); /* c = |a| */
    mp_result mp_int_neg(klisp_State *K, mp_int a, mp_int c); /* c = -a  */
/* c = a + b */
    mp_result mp_int_add(klisp_State *K, mp_int a, mp_int b, mp_int c);  
    mp_result mp_int_add_value(klisp_State *K, mp_int a, mp_small value, 
                               mp_int c);
/* c = a - b */
    mp_result mp_int_sub(klisp_State *K, mp_int a, mp_int b, mp_int c);  
    mp_result mp_int_sub_value(klisp_State *K, mp_int a, mp_small value, 
                               mp_int c);
/* c = a * b */
    mp_result mp_int_mul(klisp_State *K, mp_int a, mp_int b, mp_int c);  
    mp_result mp_int_mul_value(klisp_State *K, mp_int a, mp_small value, 
                               mp_int c);
    mp_result mp_int_mul_pow2(klisp_State *K, mp_int a, mp_small p2, mp_int c);
    mp_result mp_int_sqr(klisp_State *K, mp_int a, mp_int c); /* c = a * a */
/* q = a / b */
/* r = a % b */
    mp_result mp_int_div(klisp_State *K, mp_int a, mp_int b, mp_int q, 
                         mp_int r); 
/* q = a / value */
/* r = a % value */
    mp_result mp_int_div_value(klisp_State *K, mp_int a, mp_small value, 
                               mp_int q, mp_small *r);   
/* q = a / 2^p2  */
/* r = q % 2^p2  */
    mp_result mp_int_div_pow2(klisp_State *K, mp_int a, mp_small p2,        
                              mp_int q, mp_int r);          
/* c = a % m */
    mp_result mp_int_mod(klisp_State *K, mp_int a, mp_int m, mp_int c);  
#define   mp_int_mod_value(K, A, V, R)          \
    mp_int_div_value((K), (A), (V), 0, (R))
/* c = a^b */
    mp_result mp_int_expt(klisp_State *K, mp_int a, mp_small b, mp_int c);
/* c = a^b */
    mp_result mp_int_expt_value(klisp_State *K, mp_small a, mp_small b, mp_int c);
/* c = a^b */
    mp_result mp_int_expt_full(klisp_State *K, mp_int a, mp_int b, mp_int c);

/* NOTE: this doesn't use the allocator */
    int          mp_int_compare(mp_int a, mp_int b);                /* a <=> b        */
/* NOTE: this doesn't use the allocator */
    int          mp_int_compare_unsigned(mp_int a, mp_int b); /* |a| <=> |b| */
/* NOTE: this doesn't use the allocator */
    int          mp_int_compare_zero(mp_int z);                           /* a <=> 0  */
/* NOTE: this doesn't use the allocator */
    int          mp_int_compare_value(mp_int z, mp_small value); /* a <=> v  */

/* Returns true if v|a, false otherwise (including errors) */
    int          mp_int_divisible_value(klisp_State *K, mp_int a, mp_small v);

/* NOTE: this doesn't use the allocator */
/* Returns k >= 0 such that z = 2^k, if one exists; otherwise < 0 */
    int          mp_int_is_pow2(mp_int z);

    mp_result mp_int_exptmod(klisp_State *K, mp_int a, mp_int b, mp_int m,
                             mp_int c);                                /* c = a^b (mod m) */
    mp_result mp_int_exptmod_evalue(klisp_State *K, mp_int a, mp_small value, 
                                    mp_int m, mp_int c);   /* c = a^v (mod m) */
    mp_result mp_int_exptmod_bvalue(klisp_State *K, mp_small value, mp_int b,
                                    mp_int m, mp_int c);   /* c = v^b (mod m) */
    mp_result mp_int_exptmod_known(klisp_State *K, mp_int a, mp_int b,
                                   mp_int m, mp_int mu,
                                   mp_int c);                    /* c = a^b (mod m) */
    mp_result mp_int_redux_const(klisp_State *K, mp_int m, mp_int c); 

/* c = 1/a (mod m) */
    mp_result mp_int_invmod(klisp_State *K, mp_int a, mp_int m, mp_int c); 

/* c = gcd(a, b)   */
    mp_result mp_int_gcd(klisp_State *K, mp_int a, mp_int b, mp_int c);

/* c = gcd(a, b)   */
/* c = ax + by        */
    mp_result mp_int_egcd(klisp_State *K, mp_int a, mp_int b, mp_int c,
                          mp_int x, mp_int y);                   

/* c = lcm(a, b)   */
    mp_result mp_int_lcm(klisp_State *K, mp_int a, mp_int b, mp_int c);

/* c = floor(a^{1/b}) */
    mp_result mp_int_root(klisp_State *K, mp_int a, mp_small b, mp_int c); 
/* c = floor(sqrt(a)) */
#define   mp_int_sqrt(K, a, c) mp_int_root((K), a, 2, c)          

/* Convert to a small int, if representable; else MP_RANGE */
/* NOTE: this doesn't use the allocator */
    mp_result mp_int_to_int(mp_int z, mp_small *out);
/* NOTE: this doesn't use the allocator */
    mp_result mp_int_to_uint(mp_int z, mp_usmall *out);

/* Convert to nul-terminated string with the specified radix, writing at
   most limit characters including the nul terminator  */
    mp_result mp_int_to_string(klisp_State *K, mp_int z, mp_size radix, 
                               char *str, int limit);

/* Return the number of characters required to represent 
   z in the given radix.  May over-estimate. */
/* NOTE: this doesn't use the allocator */
    mp_result mp_int_string_len(mp_int z, mp_size radix);

/* Read zero-terminated string into z */
    mp_result mp_int_read_string(klisp_State *K, mp_int z, mp_size radix, 
                                 const char *str);
    mp_result mp_int_read_cstring(klisp_State *K, mp_int z, mp_size radix, 
                                  const char *str, char **end);

/* Return the number of significant bits in z */
/* NOTE: this doesn't use the allocator */
    mp_result mp_int_count_bits(mp_int z);

/* Convert z to two's complement binary, writing at most limit bytes */
    mp_result mp_int_to_binary(klisp_State *K, mp_int z, unsigned char *buf, 
                               int limit);

/* Read a two's complement binary value into z from the given buffer */
    mp_result mp_int_read_binary(klisp_State *K, mp_int z, unsigned char *buf, 
                                 int len);

/* Return the number of bytes required to represent z in binary. */
/* NOTE: this doesn't use the allocator */
    mp_result mp_int_binary_len(mp_int z);

/* Convert z to unsigned binary, writing at most limit bytes */
    mp_result mp_int_to_unsigned(klisp_State *K, mp_int z, unsigned char *buf, 
                                 int limit);

/* Read an unsigned binary value into z from the given buffer */
    mp_result mp_int_read_unsigned(klisp_State *K, mp_int z, unsigned char *buf, 
                                   int len);

/* Return the number of bytes required to represent z as unsigned output */
/* NOTE: this doesn't use the allocator */
    mp_result mp_int_unsigned_len(mp_int z);

/* Return a statically allocated string describing error code res */
/* NOTE: this doesn't use the allocator */
    const char *mp_error_string(mp_result res);

#if DEBUG
    void         s_print(char *tag, mp_int z);
    void         s_print_buf(char *tag, mp_digit *buf, mp_size num);
#endif

#ifdef __cplusplus
}
#endif

#endif /* end IMATH_H_ */
