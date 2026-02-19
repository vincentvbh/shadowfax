#ifndef NG_ZINT31_H
#define NG_ZINT31_H

#include "ng_mp31.h"

#include <stdint.h>

/* ==================================================================== */
/*
 * Custom bignum implementation.
 *
 * Big integers are represented as sequences of 32-bit integers; the
 * integer values are not necessarily consecutive in RAM (a dynamically
 * provided "stride" value is added to the current word pointer, to get
 * to the next word). The "len" parameter qualifies the number of words.
 *
 * Normal representation uses 31-bit limbs; each limb is stored in a
 * 32-bit word, with the top bit (31) always cleared. Limbs are in
 * low-to-high order. Signed integers use two's complement (hence, bit 30
 * of the last limb is the sign bit).
 *
 * RNS representation of a big integer x is the sequence of values
 * x modulo p, for the primes p defined in the PRIMES[] array.
 */

/*
 * Mutiply the provided big integer m with a small value x. The big
 * integer must have stride 1.
 * This function assumes that x < 2^31. The carry word is returned.
 */
uint32_t zint_mul_small(uint32_t *m, size_t len, uint32_t x);

/*
 * Reduce a big integer d modulo a small integer p.
 * Rules:
 *  d is unsigned
 *  p is prime
 *  2^30 < p < 2^31
 *  p0i = -(1/p) mod 2^31
 *  R2 = 2^64 mod p
 */
uint32_t zint_mod_small_unsigned(const uint32_t *d, size_t len, size_t stride,
    uint32_t p, uint32_t p0i, uint32_t R2);

/*
 * Similar to zint_mod_small_unsigned(), except that d may be signed.
 * Extra parameter is Rx = 2^(31*len) mod p.
 */
static inline uint32_t
zint_mod_small_signed(const uint32_t *d, size_t len, size_t stride,
    uint32_t p, uint32_t p0i, uint32_t R2, uint32_t Rx)
{
    if (len == 0) {
        return 0;
    }
    uint32_t z = zint_mod_small_unsigned(d, len, stride, p, p0i, R2);
    z = mp_sub(z, Rx & -(d[(len - 1) * stride] >> 30), p);
    return z;
}

/*
 * Add s*a to d. d and a initially have length 'len' words; the new d
 * has length 'len+1' words. 's' must fit on 31 bits. d[] and a[] must
 * not overlap. d uses stride dstride, while a has stride 1.
 */
void zint_add_mul_small(uint32_t *restrict d, size_t len, size_t dstride,
    const uint32_t *restrict a, uint32_t s);

/*
 * Normalize a modular integer around 0: if x > p/2, then x is replaced
 * with x - p (signed encoding with two's complement); otherwise, x is
 * untouched. The two integers x and p are encoded over the same length;
 * x has stride xstride, while p has stride 1.
 */
void zint_norm_zero(uint32_t *restrict x, size_t len, size_t xstride,
    const uint32_t *restrict p);

/*
 * Rebuild integers from their RNS representation. There are 'num_sets' sets
 * of 'n' integers. Within each set, the n integers are interleaved,
 * so that words of a given integer occur every n slots in RAM (i.e. each
 * integer has stride 'n'). The sets are consecutive in RAM.
 *
 * If "normalize_signed" is non-zero, then the output values are
 * normalized to the -m/2..m/2 interval (where m is the product of all
 * small prime moduli); two's complement is used for negative values.
 * If "normalize_signed" is zero, then the output values are all
 * in the 0..m-1 range.
 *
 * tmp[] must have room for xlen words.
 */
void zint_rebuild_CRT(uint32_t *restrict xx, size_t xlen, size_t n,
    size_t num_sets, int normalize_signed, uint32_t *restrict tmp);

/*
 * Negate a big integer conditionally: value a is replaced with -a if
 * and only if ctl = 1. Control value ctl must be 0 or 1. The integer
 * has stride 1.
 */
void zint_negate(uint32_t *a, size_t len, uint32_t ctl);

/*
 * Get the number of leading zeros in a 32-bit value.
 */
static inline unsigned
lzcnt(uint32_t x)
{
    uint32_t m = tbmask((x >> 16) - 1);
    uint32_t s = m & 16;
    x = (x >> 16) ^ (m & (x ^ (x >> 16)));
    m = tbmask((x >>  8) - 1);
    s |= m &  8;
    x = (x >>  8) ^ (m & (x ^ (x >>  8)));
    m = tbmask((x >>  4) - 1);
    s |= m &  4;
    x = (x >>  4) ^ (m & (x ^ (x >>  4)));
    m = tbmask((x >>  2) - 1);
    s |= m &  2;
    x = (x >>  2) ^ (m & (x ^ (x >>  2)));

    /*
     * At this point, x fits on 2 bits. Number of leading zeros is
     * then:
     *    x = 0   -> 2
     *    x = 1   -> 1
     *    x = 2   -> 0
     *    x = 3   -> 0
     */
    return (unsigned)(s + ((2 - x) & tbmask(x - 3)));
}

/*
 * Identical to lzcnt(), except that the caller makes sure that the
 * operand is non-zero. On (old-ish) x86 systems, this function could be
 * specialized with the bsr opcode (which does not support a zero input).
 */
#define lzcnt_nonzero   lzcnt

/*
 * Compute a GCD between two positive big integers x and y. The two
 * integers must be odd. Returned value is 1 if the GCD is 1, 0
 * otherwise. When 1 is returned, arrays u and v are filled with values
 * such that:
 *   0 <= u <= y
 *   0 <= v <= x
 *   x*u - y*v = 1
 * x[] and y[] are unmodified. Both input values must have the same
 * encoded length. Temporary array must be large enough to accommodate 4
 * extra values of that length. Arrays u, v and tmp may not overlap with
 * each other, or with either x or y. All integers use stride 1.
 */
int zint_bezout(uint32_t *restrict u, uint32_t *restrict v,
    const uint32_t *restrict x, const uint32_t *restrict y,
    size_t len, uint32_t *restrict tmp);

/*
 * Add k*(2^sc)*y to x. The result is assumed to fit in the array of
 * size xlen (truncation is applied if necessary).
 * Scale factor sc is provided as sch and scl, such that:
 *    sch = sc / 31
 *    scl = sc % 31  (in the 0..30 range)
 * xlen MUST NOT be lower than ylen; however, it is allowed that
 * xlen is greater than ylen.
 *
 * x[] and y[] are both signed integers, using two's complement for
 * negative values. They both use the same stride ('stride' parameter).
 */
void zint_add_scaled_mul_small(uint32_t *restrict x, size_t xlen,
    const uint32_t *restrict y, size_t ylen, size_t stride,
    int32_t k, uint32_t sch, uint32_t scl);

/*
 * Subtract y*2^sc from x. This is a specialized version of
 * zint_add_scaled_mul_small(), with multiplier k = -1.
 */
void zint_sub_scaled(uint32_t *restrict x, size_t xlen,
    const uint32_t *restrict y, size_t ylen, size_t stride,
    uint32_t sch, uint32_t scl);


#endif

