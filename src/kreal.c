/*
** kreal.c
** Kernel Reals (doubles)
** See Copyright Notice in klisp.h
*/

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h> 
#include <math.h>
#include <fenv.h> /* for setting round direction */

#include "kreal.h"
#include "krational.h"
#include "kinteger.h"
#include "kobject.h"
#include "kstate.h"
#include "kmem.h"
#include "kgc.h"
#include "kpair.h" /* for list in throw error */
#include "kerror.h"

double kbigint_to_double(Bigint *bigint)
{
    double radix = (double) UINT32_MAX + 1.0;
    uint32_t ndigits = bigint->used;
    double accum = 0.0;
	
    /* bigint is in little endian format, but we traverse in
       big endian */
    do {
        --ndigits;
        accum = accum * radix + (double) bigint->digits[ndigits];
    } while (ndigits > 0); /* have to compare like this, it's unsigned */
    return mp_int_compare_zero(bigint) < 0? -accum : accum;
}

/* bigrat is rooted */
double kbigrat_to_double(klisp_State *K, Bigrat *bigrat)
{	
    /* TODO: check rounding in extreme cases */
    TValue tv_rem = kbigrat_copy(K, gc2bigrat(bigrat));
    krooted_tvs_push(K, tv_rem);
    Bigrat *rem = tv2bigrat(tv_rem);
    UNUSED(mp_rat_abs(K, rem, rem)); 

    TValue int_radix = kbigint_make_simple(K);
    krooted_tvs_push(K, int_radix);
    /* cant do UINT32_MAX and then +1 because _value functions take 
       int32_t arguments */ 
    UNUSED(mp_int_set_value(K, tv2bigint(int_radix), INT32_MAX)); 
    UNUSED(mp_int_add_value(K, tv2bigint(int_radix), 1, 
                            tv2bigint(int_radix)));
    UNUSED(mp_int_add(K, tv2bigint(int_radix), tv2bigint(int_radix), 
                      tv2bigint(int_radix)));

    TValue int_part = kbigint_make_simple(K);
    krooted_tvs_push(K, int_part);

    double accum = 0.0;
    double radix = (double) UINT32_MAX + 1.0;
    uint32_t digit = 0;
    /* inside there is a check to avoid problem with continuing fractions */
    while(mp_rat_compare_zero(rem) > 0) {
        UNUSED(mp_int_div(K, MP_NUMER_P(rem), MP_DENOM_P(rem), 
                          tv2bigint(int_part), NULL));

        double di = kbigint_to_double(tv2bigint(int_part));
        bool was_zero = di == 0.0;
        for (uint32_t i = 0; i < digit; i++) {
            di /= radix;
        }
        /* if last di became 0.0 we will exit next loop,
           this is to avoid problem with continuing fractions */
        if (!was_zero && di == 0.0)
            break;

        ++digit;
        accum += di; 
	
        UNUSED(mp_rat_sub_int(K, rem, tv2bigint(int_part), rem));
        UNUSED(mp_rat_mul_int(K, rem, tv2bigint(int_radix), rem));
    }
    krooted_tvs_pop(K); /* int_part */
    krooted_tvs_pop(K); /* int_radix */
    krooted_tvs_pop(K); /* rem */

    return mp_rat_compare_zero(bigrat) < 0? -accum : accum;
}

/* TODO test strict arithmetic and throw errors on overflow and underflow? 
   if set */
TValue kexact_to_inexact(klisp_State *K, TValue n)
{
    bool strictp = kcurr_strict_arithp(K);

    switch(ttype(n)) {
    case K_TFIXINT:
        /* NOTE: can't over or underflow, and can't give NaN */
        return d2tv((double) ivalue(n));
    case K_TBIGINT: {
        Bigint *bigint = tv2bigint(n);
        double d = kbigint_to_double(bigint);
        if (strictp && (d == 0.0 || isinf(d) || isnan(d))) {
            /* NOTE: bigints can't be zero */
            char *msg;
            if (isnan(d))
                msg = "unexpected error";
            else if (isinf(d))
                msg = "overflow";
            else
                msg = "undeflow";
            klispE_throw_simple_with_irritants(K, msg, 1, n);
            return KUNDEF;
        } else {
            /* d may be inf, ktag_double will handle it */
            return ktag_double(d);
        }
    }
    case K_TBIGRAT: {
        Bigrat *bigrat = tv2bigrat(n);
        double d = kbigrat_to_double(K, bigrat);
        /* REFACTOR: this code is the same for bigints... */
        if (strictp && (d == 0.0 || isinf(d) || isnan(d))) {
            /* NOTE: bigrats can't be zero */
            char *msg;
            if (isnan(d))
                msg = "unexpected error";
            else if (isinf(d))
                msg = "overflow";
            else
                msg = "undeflow";
            klispE_throw_simple_with_irritants(K, msg, 1, n);
            return KUNDEF;
        } else {
            /* d may be inf, ktag_double will handle it */
            return ktag_double(d);
        }
    }
    case K_TEINF:
        return tv_equal(n, KEPINF)? KIPINF : KIMINF;
        /* all of these are already inexact */
    case K_TDOUBLE:
    case K_TIINF:
    case K_TRWNPV:
    case K_TUNDEFINED:
        return n;
    default:
        klisp_assert(0);
        return KUNDEF;
    }
}

/* assume d is integer and doesn't fit in a fixint */
TValue kdouble_to_bigint(klisp_State *K, double d)
{
    bool neg = d < 0;
    if (neg)
        d = -d;

    TValue tv_res = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_res);
    Bigint *res = tv2bigint(tv_res);
    mp_int_set_value(K, res, 0);

    TValue tv_digit = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_digit);
    Bigint *digit = tv2bigint(tv_digit);

    /* do it 32 bits at a time */
    double radix = ((double) UINT32_MAX) + 1.0;
    int power =  0;
    while(d > 0) {
        double dd = fmod(d, radix);
        d = floor(d / radix);
        /* load in two moves because set_value takes signed ints */
        uint32_t id = (uint32_t) dd;
        int32_t id1 = (int32_t) (id >> 1);
        int32_t id2 = (int32_t) (id - id1);

        mp_int_set_value(K, digit, id1);
        mp_int_add_value(K, digit, id2, digit);

        mp_int_mul_pow2(K, digit, power, digit);
        mp_int_add(K, res, digit, res);

        power += 32;
    }

    if (neg)
        mp_int_neg(K, res, res);

    krooted_tvs_pop(K); /* digit */
    krooted_tvs_pop(K); /* res */

    return tv_res; /* can't be fixint except when coming from
                      kdouble_to_bigrat, so don't convert */
}

/* TODO: should do something like rationalize with range +/- 1/2ulp) */
TValue kdouble_to_bigrat(klisp_State *K, double d)
{
    bool neg = d < 0;
    if (neg)
        d = -d;

    /* find an integer, convert it and divide by 
       an adequate power of 2 */
    TValue tv_res = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_res);
    Bigrat *res = tv2bigrat(tv_res);
    UNUSED(mp_rat_set_value(K, res, 0, 1));

    TValue tv_den = kbigint_make_simple(K);
    krooted_tvs_push(K, tv_den);
    Bigint *den = tv2bigint(tv_den);
    UNUSED(mp_int_set_value(K, den, 1));

    /* XXX could be made a lot more efficiently reading ieee
       fields directly */
    int ie;
    d = frexp(d, &ie);

    while(d != floor(d)) {
        d *= 2.0;
        --ie;
    }
    UNUSED(mp_int_mul_pow2(K, den, -ie, den));

    TValue tv_num = kdouble_to_bigint(K, d);
    krooted_tvs_push(K, tv_num);
    Bigint *num = tv2bigint(tv_num);

    TValue tv_den2 = kbigrat_make_simple(K);
    krooted_tvs_push(K, tv_den2);
    Bigrat *den2 = tv2bigrat(tv_den2);

    UNUSED(mp_rat_set_value(K, den2, 0, 1));
    UNUSED(mp_rat_add_int(K, den2, den, den2));
    UNUSED(mp_rat_set_value(K, res, 0, 1));
    UNUSED(mp_rat_add_int(K, res, num, res));
    UNUSED(mp_rat_div(K, res, den2, res));

    if (neg)
        UNUSED(mp_rat_neg(K, res, res));

    /* now create a value corresponding to 1/2 ulp
       for rationalize */
    UNUSED(mp_int_mul_pow2(K, den, 1, den));
    UNUSED(mp_rat_set_value(K, den2, 0, 1));
    UNUSED(mp_rat_add_int(K, den2, den, den2));
    UNUSED(mp_rat_recip(K, den2, den2));
    /* den2 now has 1/2 ulp */

    TValue rationalized = kbigrat_rationalize(K, tv_res, tv_den2);

    krooted_tvs_pop(K); /* den2 */
    krooted_tvs_pop(K); /* num */
    krooted_tvs_pop(K); /* den */
    krooted_tvs_pop(K); /* res */

    return rationalized;
}

TValue kinexact_to_exact(klisp_State *K, TValue n)
{
    switch(ttype(n)) {
    case K_TFIXINT:
    case K_TBIGINT:
    case K_TBIGRAT:
    case K_TEINF:
        /* all of these are already exact */
        return n;
    case K_TDOUBLE: {
        double d = dvalue(n);
        klisp_assert(!isnan(d) && !isinf(d));
        if (d == floor(d)) { /* integer */
            if (d <= (double) INT32_MAX && 
                d >= (double) INT32_MIN) {
                return i2tv((int32_t) d); /* fixint */
            } else {
                return kdouble_to_bigint(K, d);
            }
        } else { /* non integer rational */
            return kdouble_to_bigrat(K, d);
        }
    }
    case K_TIINF:
        return tv_equal(n, KIPINF)? KEPINF : KEMINF;
    case K_TRWNPV:
    case K_TUNDEFINED:
        klispE_throw_simple_with_irritants(K, "no primary value", 1, n);
        return KUNDEF;
    default:
        klisp_assert(0);
        return KUNDEF;
    }
}

/*
** read/write interface 
*/

/*
** SOURCE NOTE: This is a more or less vanilla implementation of the algorithm
** described in "How to Print Floating-Point Numbers Accurately" by 
** Guy L. Steele Jr. & John L. White.
*/

/*
** TODO add awareness of read rounding (e.g. problem with 1.0e23)
** TODO add exponent if too small or too big
*/

mp_result shift_2(klisp_State *K, Bigint *x, Bigint *n, Bigint *r)
{
    mp_small nv;
    mp_result res = mp_int_to_int(n, &nv);
    klisp_assert(res == MP_OK);

    if (nv >= 0)
        return mp_int_mul_pow2(K, x, nv, r);
    else
        return mp_int_div_pow2(K, x, -nv, r, NULL);
}

/* returns k, modifies all parameters (except f & p) */
int32_t simple_fixup(klisp_State *K, Bigint *f, Bigint *p, Bigint *r, 
                     Bigint *s, Bigint *mm, Bigint *mp)
{
    mp_result res;
    Bigint tmpz, tmpz2;
    Bigint *tmp = &tmpz;
    Bigint *tmp2 = &tmpz2;
    Bigint onez;
    Bigint *one = &onez;
    res = mp_int_init(tmp);
    res = mp_int_init(tmp2);
    res = mp_int_init_value(K, one, 1);
    res = mp_int_sub(K, p, one, tmp);
    res = shift_2(K, one, tmp, tmp);

    if (mp_int_compare(f, tmp) == 0) {
        res = shift_2(K, mp, one, mp);
        res = shift_2(K, r, one, r);
        res = shift_2(K, s, one, s);
    }

    int k = 0;

    /* tmp = ceiling (s/10), for while guard */
    res = mp_int_add_value(K, s, 9, tmp);
    res = mp_int_div_value(K, tmp, 10, tmp, NULL);

    while(mp_int_compare(r, tmp) < 0) {
        --k;
        res = mp_int_mul_value(K, r, 10, r);
        res = mp_int_mul_value(K, mm, 10, mm);
        res = mp_int_mul_value(K, mp, 10, mp);
        /* tmp = ceiling (s/10), for while guard */
        res = mp_int_add_value(K, s, 9, tmp);
        res = mp_int_div_value(K, tmp, 10, tmp, NULL);
    }

    /* tmp = 2r + mp; tmp2 = 2s */
    res = mp_int_mul_value(K, r, 2, tmp);
    res = mp_int_add(K, tmp, mp, tmp);
    res = mp_int_mul_value(K, s, 2, tmp2);
    while(mp_int_compare(tmp, tmp2) >= 0) {

        res = mp_int_mul_value(K, s, 10, s);
        ++k;

        /* tmp = 2r + mp; tmp2 = 2s */
        res = mp_int_mul_value(K, r, 2, tmp);
        res = mp_int_add(K, tmp, mp, tmp);
        res = mp_int_mul_value(K, s, 2, tmp2);
    }

    mp_int_clear(K, tmp);
    mp_int_clear(K, tmp2);
    mp_int_clear(K, one);

    UNUSED(res);
    return k;
}

/* TEMP: for now upoint is passed indicating where the least significant
   integer digit should be (10^0 position) */
#define digit_pos(k_, upoint_) ((k_) + (upoint_))

bool dtoa(klisp_State *K, double d, char *buf, int32_t upoint, int32_t *out_h, 
          int32_t *out_k)
{
    klisp_assert(sizeof(mp_small) == 4);
    mp_result res;
    Bigint e, p, f;

    klisp_assert(d > 0.0);

    /* convert d to three bigints m: significand, e: exponent & p: precision */
    /* d = m^(e-p) & m < 2^p */
    int ie, ip;
    double mantissa = frexp(d, &ie);
    ip = 0;

    klisp_assert(mantissa * pow(2.0, ie) == d);
    /* now 0.5 <= mantissa < 1 & mantissa * 2^expt = d */
/* this could be a binary search, it could also be done reading the exponent
   field of ieee754 directly... */
    while(mantissa != floor(mantissa)) { 
        mantissa *= 2.0;
        ++ip;
    }
	
    /* mantissa is int & < 2^ip (was < 1=2^0 and by induction...) */
    klisp_assert(mantissa * pow(2.0, ie - ip) == d);
    /* mantissa is at most 53 bits long as an int, load it in two parts
       to f */
    int64_t im = (int64_t) mantissa;
    /* f */
    /* cant load 32 bits at a time, second param is signed!,
       but we know it's positive so load 32 then 31 */
    res = mp_int_init_value(K, &f, (mp_small) (im >> 31));
    res = mp_int_mul_pow2(K, &f, 31, &f);
    res = mp_int_add_value(K, &f, (mp_small) im & 0x7fffffff, &f);

    /* adjust f & p so that p is 53 TODO do in one step */
    /* XXX: is this is ok for denorms?? */
    while(ip < 53) {
        ++ip;
        res = mp_int_mul_value(K, &f, 2, &f);
    }
    
    /* e */
    res = mp_int_init_value(K, &e, (mp_small) ie);

    /* p */
    res = mp_int_init_value(K, &p, (mp_small) ip);

    /* start of FPP^2 algorithm */
    Bigint r, s;
    Bigint mp, mm;
    Bigint e_p;
    Bigint zero, one;

    res = mp_int_init_value(K, &zero, 0);
    res = mp_int_init_value(K, &one, 1);

    res = mp_int_init(&r);
    res = mp_int_init(&s);
    res = mp_int_init(&mm);
    res = mp_int_init(&mp);
    res = mp_int_init(&e_p);

    res = mp_int_sub(K, &e, &p, &e_p);

//    shift_2(f, max(e-p, 0), r);
//    shift_2(1, max(-(e-p), 0), r);
    if (mp_int_compare_zero(&e_p) >= 0) {
        res = shift_2(K, &f, &e_p, &r);
        res = shift_2(K, &one, &zero, &s); /* nop */
        res = shift_2(K, &one, &e_p, &mm);
    } else {
        res = shift_2(K, &f, &zero, &r); /* nop */
        res = mp_int_neg(K, &e_p, &e_p); 
        res = shift_2(K, &one, &e_p, &s);
        res = shift_2(K, &one, &zero, &mm);
    }
    mp_int_copy(K, &mm, &mp);

    int32_t k = simple_fixup(K, &f, &p, &r, &s, &mm, &mp);
    int32_t h = k-1;

    Bigint u, tmp, tmp2;
    res = mp_int_init(&u);
    res = mp_int_init(&tmp);
    res = mp_int_init(&tmp2);
    bool low, high;

    do {
        --k;
        res = mp_int_mul_value(K, &r, 10, &tmp);
        res = mp_int_div(K, &tmp, &s, &u, &r);
        res = mp_int_mul_value(K, &mm, 10, &mm);
        res = mp_int_mul_value(K, &mp, 10, &mp);

        /* low/high flags */
        /* XXX try to make 1e23 round correctly,
           it causes tmp == tmp2 but should probably
           check oddness of digit and (may result in a digit
           with value 10?, needing to backtrack) 
           In general make it so that if rounding done at reading
           (should be round to even) is accounted for and the minimal
           length number is generated */

        res = mp_int_mul_value(K, &r, 2, &tmp);

        low = mp_int_compare(&tmp, &mm) < 0;

        res = mp_int_mul_value(K, &s, 2, &tmp2);
        res = mp_int_sub(K, &tmp2, &mp, &tmp2);

        high = mp_int_compare(&tmp, &tmp2) > 0;

        if (!low && !high) {
            mp_small digit;
            res = mp_int_to_int(&u, &digit);
            klisp_assert(res == MP_OK);
            klisp_assert(digit >= 0 && digit <= 9);
            buf[digit_pos(k, upoint)] = '0' + digit;
        }
    } while(!low && !high);
    
    mp_small digit;
    res = mp_int_to_int(&u, &digit);
    klisp_assert(res == MP_OK);
    klisp_assert(digit >= 0 && digit <= 9);

    if (low && high) {
        res = mp_int_mul_value(K, &r, 2, &tmp);
        int cmp = mp_int_compare(&tmp, &s);
        if ((cmp == 0 && (digit & 1) != 0) || cmp > 0)
            ++digit;
    } else if (low) {
        /* nothing */
    } else if (high) {
        ++digit;
    } else {
        klisp_assert(0);
    }
    /* double check in case there was an increment */
    klisp_assert(digit >= 0 && digit <= 9);
    buf[digit_pos(k, upoint)] = '0' + digit;

    *out_h = h;
    *out_k = k;
    /* add '\0' to both sides */
    buf[digit_pos(k-1, upoint)] = '\0';
    buf[digit_pos(h+1, upoint)] = '\0';

    mp_int_clear(K, &f);
    mp_int_clear(K, &e);
    mp_int_clear(K, &p);
    mp_int_clear(K, &r);
    mp_int_clear(K, &s);
    mp_int_clear(K, &mp);
    mp_int_clear(K, &mm);
    mp_int_clear(K, &e_p);
    mp_int_clear(K, &zero);
    mp_int_clear(K, &one);
    mp_int_clear(K, &u);
    mp_int_clear(K, &tmp);
    mp_int_clear(K, &tmp2);

    /* NOTE: digits are reversed! */
    return true;
}


/* TEMP: this is a stub for now, always return sufficiently large 
   number */
int32_t kdouble_print_size(TValue tv_double)
{
    klisp_assert(ttisdouble(tv_double));
    UNUSED(tv_double);
    return 1024;
}

void kdouble_print_string(klisp_State *K, TValue tv_double,
                          char *buf, int32_t limit)
{
    klisp_assert(ttisdouble(tv_double));
    /* TODO: add exponent to values too large or too small */
    /* TEMP */
    int32_t h = 0;
    int32_t k = 0;
    int32_t upoint = limit/2;
    double od = dvalue(tv_double);
    klisp_assert(!isnan(od) && !isinf(od));
    klisp_assert(limit > 10);

    /* dtoa only works for d > 0 */
    if (od == 0.0) {
        buf[0] = '0';
        buf[1] = '.';
        buf[2] = '0';
        buf[3] = '\0';
        return;
    } 
    
    double d;
    if (od < 0.0)
        d = -od;
    else d = od;

    /* XXX this doesn't check limit, it should be large enough */
    UNUSED(dtoa(K, d, buf, upoint, &h, &k));

    klisp_assert(upoint + k >= 0 && upoint + h + 1 < limit);

    /* rearrange the digits */
    /* TODO do this more efficiently */


    int32_t size = h - k + 1;
    int32_t start = upoint+k;
    /* first reverse the digits */
    for (int32_t i = upoint+k, j = upoint+h; i < j; i++, j--) {
        char ch = buf[i];
        buf[i] = buf[j];
        buf[j] = ch;
    }

    /* TODO use exponents */

    /* if necessary make room for leading zeros and sign,
       move all to the left */
    
    int extra_size = (od < 0? 1 : 0) + (h < 0? 2 + (-h-1) : 0);

    klisp_assert(extra_size + size + 2 < limit); 
    memmove(buf+extra_size, buf+start, size);
    
    int32_t i = 0;
    if (od < 0)
        buf[i++] = '-';

    if (h < 0) {
        /* fraction with leading 0. and with possibly more leading zeros */
        buf[i++] = '0';
        buf[i++] = '.';
        for (int32_t j = -1; j > h; j--) {
            buf[i++] = '0';
        }
        int frac_size = size;
        i += frac_size;
        buf[i++] = '\0';
    } else if (k >= 0) {
        /* integer with possibly trailing zeros */
        klisp_assert(size+extra_size+k+4 < limit);
        int int_size = size;
        i += int_size;
        for (int32_t j = 0; j < k; j++) {
            buf[i++] = '0';
        }
        buf[i++] = '.';
        buf[i++] = '0';
        buf[i++] = '\0';
    } else { /* both integer and fractional part, make room for the point */
        /* k < 0, h >= 0 */
        int32_t int_size = h+1;
        int32_t frac_size = -k;
        memmove(buf+i+int_size+1, buf+i+int_size, frac_size);
        i += int_size;
        buf[i++] = '.';
        i += frac_size;
        buf[i++] = '\0';
    }
    return;
}

double kdouble_div_mod(double n, double d, double *res_mod) 
{
    double div = floor(n / d);
    double mod = fmod(n, d);

    /* div, mod or div-and-mod */
    /* 0 <= mod0 < |d| */
    if (mod < 0.0) {
        if (d < 0.0) {
            mod -= d;
            div += 1.0;
        } else {
            mod += d;
            div -= 1.0;
        }
    }
    *res_mod = mod;
    return div;
}

double kdouble_div0_mod0(double n, double d, double *res_mod) 
{
    double div = floor(n / d);
    double mod = fmod(n, d);

    /* div0, mod0 or div-and-mod0 */
    /*
    ** Adjust q and r so that:
    ** -|d/2| <= mod0 < |d/2| which is the same as
    ** dmin <= mod0 < dmax, where 
    ** dmin = -|d/2| and dmax = |d/2| 
    */
    double dmin = -((d<0.0? -d : d) / 2.0);
    double dmax = (d<0.0? -d : d) / 2.0;
	
    if (mod < dmin) {
        if (d < 0) {
            mod -= d;
            div += 1.0;
        } else {
            mod += d;
            div -= 1.0;
        }
    } else if (mod >= dmax) {
        if (d < 0) {
            mod += d;
            div += 1.0;
        } else {
            mod -= d;
            div -= 1.0;
        }
    }
    *res_mod = mod;
    return div;
}

TValue kdouble_to_integer(klisp_State *K, TValue tv_double, kround_mode mode)
{
    double d = dvalue(tv_double);
    switch(mode) {
    case K_TRUNCATE: 
        d = trunc(d);
        break;
    case K_CEILING:
        d = ceil(d);
        break;
    case K_FLOOR:
        d = floor(d);
        break;
    case K_ROUND_EVEN: {
        int res = fesetround(FE_TONEAREST); /* REFACTOR: should be done once only... */
        klisp_assert(res == 0);
        d = nearbyint(d);
        break;
    }
    }
    /* ASK John: we currently return inexact if given inexact is this ok?
       or should it return an exact integer? */
    return ktag_double(d);
#if 0    
    tv_double = ktag_double(d); /* won't alloc mem so no need to root */
    return kinexact_to_exact(K, tv_double); 
#endif
}
