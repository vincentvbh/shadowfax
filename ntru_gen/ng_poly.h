#ifndef NG_POLY_H
#define NG_POLY_H

#include "ng_fxp.h"

#include <stdint.h>

/* ==================================================================== */
/*
 * Code for polynomials with integer coefficients.
 *
 * Polynomials use an interleaved in-memory representation:
 *
 *   There are n = 2^logn coefficients (degrees 0 to n-1).
 *   Each coefficient contains 'len' words (zint limbs or RNS).
 *   Each coefficient has a stride of 'n'.
 *   The first (lowest) words of the respective coefficients are consecutive.
 */

/*
 * Load a one-byte polynomial with reduction modulo p.
 */
void poly_mp_set_small(unsigned logn, uint32_t *restrict d,
    const int8_t *restrict f, uint32_t p);

/*
 * Convert a polynomial in one-word normal representation (signed) into RNS
 * modulo the single prime p.
 */
void poly_mp_set(unsigned logn, uint32_t *f, uint32_t p);

/*
 * Convert a polynomial in RNS (modulo a single prime p) into one-word
 * normal representation (signed).
 */
void poly_mp_norm(unsigned logn, uint32_t *f, uint32_t p);

/*
 * Convert a polynomial to small integers. Source values are supposed
 * to be normalized (signed). Returned value is 0 if any of the
 * coefficients exceeds the provided limit (in absolute value); on
 * success, 1 is returned.
 *
 * In case of failure, the function returns earlier; this does not
 * break constant-time discipline as long as a failure implies that the
 * (f,g) polynomials are discarded.
 */
int poly_big_to_small(unsigned logn, int8_t *restrict d,
    const uint32_t *restrict s, int lim);

/*
 * Get the maximum bit length of all coefficients of a polynomial. Each
 * coefficient has size flen words.
 *
 * The bit length of a big integer is defined to be the length of the
 * minimal binary representation, using two's complement for negative
 * values, and excluding the sign bit. This definition implies that
 * if x = 2^k, then x has bit length k but -x has bit length k-1. For
 * non powers of two, x and -x have the same bit length.
 *
 * This function is constant-time with regard to coefficient values and
 * the returned bit length.
 */
uint32_t poly_max_bitlength(unsigned logn, const uint32_t *f, size_t flen);

/*
 * Compute q = x / 31 and r = x % 31 for an unsigned integer x. This
 * macro is constant-time and works for values x up to 63487 (inclusive).
 */
#define DIVREM31(q, r, x)  { \
        uint32_t divrem31_q, divrem31_x; \
        divrem31_x = (x); \
        divrem31_q = (uint32_t)(divrem31_x * (uint32_t)67651) >> 21; \
        (q) = divrem31_q; \
        (r) = divrem31_x - 31 * divrem31_q; \
    } while (0)

/*
 * Convert a polynomial to fixed-point approximations, with scaling.
 * For each coefficient x, the computed approximation is x/2^sc.
 * This function assumes that |x| < 2^(30+sc). The length of each
 * coefficient must be less than 2^24 words.
 *
 * This function is constant-time with regard to the coefficient values
 * and to the scaling factor.
 */
void poly_big_to_fixed(unsigned logn, fxr *restrict d,
    const uint32_t *restrict f, size_t len, uint32_t sc);

/*
 * Subtract k*f from F, where F, f and k are polynomials modulo X^n+1.
 * Coefficients of polynomial k are small integers (signed values in the
 * -2^31..+2^31 range) scaled by 2^sc.
 *
 * This function implements the basic quadratic multiplication algorithm,
 * which is efficient in space (no extra buffer needed) but slow at
 * high degree.
 */
void poly_sub_scaled(unsigned logn,
    uint32_t *restrict F, size_t Flen,
    const uint32_t *restrict f, size_t flen,
    const int32_t *restrict k, uint32_t sc);

/*
 * Subtract k*f from F. Coefficients of polynomial k are small integers
 * (signed values in the -2^31..+2^31 range) scaled by 2^sc. Polynomial f
 * MUST be in RNS+NTT over flen+1 words (even though f itself would fit on
 * flen words); polynomial F MUST be in plain representation.
 */
void
poly_sub_scaled_ntt(unsigned logn, uint32_t *restrict F, size_t Flen,
    const uint32_t *restrict f, size_t flen,
    const int32_t *restrict k, uint32_t sc, uint32_t *restrict tmp);

/*
 * depth = 1
 * logn = logn_top - depth
 * Inputs:
 *    F, G    polynomials of degree 2^logn, plain integer representation (FGlen)
 *    FGlen   size of each coefficient of F and G (must be 1 or 2)
 *    f, g    polynomials of degree 2^logn_top, small coefficients
 *    k       polynomial of degree 2^logn (plain, 32-bit)
 *    sc      scaling logarithm (public value)
 *    tmp     temporary with room at least max(FGlen, 2^logn_top) words
 * Operation:
 *    F <- F - (2^sc)*k*ft
 *    G <- G - (2^sc)*k*gt
 * with (ft,gt) being the degree-n polynomials corresponding to (f,g)
 * It is assumed that the result fits.
 *
 * WARNING: polynomial k is consumed in the process.
 *
 * This function uses 3*n words in tmp[].
 */
void poly_sub_kfg_scaled_depth1(unsigned logn_top,
    uint32_t *restrict F, uint32_t *restrict G, size_t FGlen,
    uint32_t *restrict k, uint32_t sc,
    const int8_t *restrict f, const int8_t *restrict g,
    uint32_t *restrict tmp);

/*
 * Check whether the provided polynomial is invertible modulo X^n+1
 * and modulo some small prime r which is such that r-1 is a multiple
 * of 2048. Parameters:
 *    logn     degree logarithm
 *    f        polynomial to test
 *    r        small prime to use
 *    p        p = r*t, such that t is prime and (4/3)*2^30 < p < 2^31
 *    p0i      -1/p mod 2^32
 *    s        s'*2^32 mod p, for some s' such that s'^1024 = -1 mod r
 *    rm, rs   division by r parameters
 *    tmp      temporary buffer
 * rm and rs must be such that floor(x*rm/(2^rs)) == floor(x/r) for all
 * x in [0..p-1]; such values always exist.
 *
 * Return value: 1 if f is invertible, 0 otherwise.
 *
 * RAM USAGE: 2*n words
 */
int
poly_is_invertible(unsigned logn, const int8_t *restrict f,
    uint32_t p, uint32_t p0i, uint32_t s,
    uint32_t r, uint32_t rm, unsigned rs, uint32_t *restrict tmp);

/*
 * Similar to poly_is_invertible(), except that we test two prime moduli
 * at the same time. We have p = r1*r2*t, with both r1-1 and r2-1 being
 * multiples of 2048, and s is a 2048-th root of 1 modulo both r1 and r2
 * (s is in Montgomery representation).
 */
int
poly_is_invertible_ext(unsigned logn, const int8_t *restrict f,
    uint32_t r1, uint32_t r2, uint32_t p, uint32_t p0i, uint32_t s,
    uint32_t r1m, unsigned r1s, uint32_t r2m, unsigned r2s,
    uint32_t *restrict tmp);

uint32_t poly_sqnorm(unsigned logn, const int8_t *f);

#endif

