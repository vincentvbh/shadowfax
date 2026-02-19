#ifndef NG_MP31_H
#define NG_MP31_H

#include <stdint.h>
#include <stddef.h>

/*
 * Expand the top bit of value x into a full 32-bit mask (i.e. return
 * 0xFFFFFFFF if x >= 0x80000000, or 0x00000000 otherwise).
 */
static inline uint32_t
tbmask(uint32_t x)
{
    return (uint32_t)(*(int32_t *)&x >> 31);
}

/* ==================================================================== */
/*
 * Modular arithmetics.
 *
 * We implement computations modulo some small integer p with the following
 * characteristics:
 *
 *   (4/3)*2^30 < p < 2^31       (this implies that 2*p < 2^32 < 3*p)
 *   p-1 is a multiple of 2048
 *
 * Operands are held in 32-bit values (uint32_t). We define R = 2^32 mod p.
 *
 * Values modulo p are 32-bit integers (uint32_t type) in the 0 to p-1 range.
 * Montgomery representation of an element x of Z_p is the value x*R mod p
 * (also in the 0 to p-1 range). Montgomery multiplication of x and y
 * computes x*y/R mod p (thus, Montgomery multiplication of the Montgomery
 * representations of x and y outputs the Montgomery representation of the
 * product x*y, since (x*R)*(y*R)/R = (x*y)*R). In general, values are not
 * kept in Montgomery representation, unless explicitly specified.
 *
 * The "signed normalized" value of x modulo p is the unique integer v
 * in the -(p-1)/2 to +(p-1)/2 range such that x = v mod p.
 *
 * The PRIMES[] array contains the largest of such primes, in
 * descending order. Six values are provided for each prime p:
 *    p     modulus
 *    p0i   -1/p mod 2^32
 *    R2    2^64 mod p
 *    g     a primitive 2048-th root of 1 modulo p (i.e. g^1024 = -1 mod p)
 *    ig    1/g mod p
 *    s     inverse mod p of the product of the previous primes
 * Values g, ig and s are in Montgomery representation.
 * R2 is used to convert values to Montgomery representations
 * (with montymul(x, R2) = x*R2/R = x*R mod p). g and ig are used to
 * generate the tables used for NTT and inverse NTT. Value s supports
 * reconstruction of a big integer in RNS representation with the CRT.
 * The product of all the primes in the PRIMES[] array is about 2^10012.25
 * and thus appropriate for big integers (in RNS) of up to 10000 bits.
 *
 * Polynomials over Z_p are considered modulo X^n+1 for n a power of two
 * between 2 and 1024 (inclusive). The degree n is provided as parameter
 * 'logn' in the 1 to 10 range (with n = 2^logn). The polynomial
 * coefficients are consecutive in RAM, in ascending degree order. The
 * NTT representation of such a polynomial is the evaluation of
 * the polynomial over the roots of X^n+1 (which are (g^(1024/n))^(2*i+1)
 * for integers i = 0 to n-1).
 *
 * Non-prime modulus
 * -----------------
 *
 * This code also works for some non-prime moduli p. If p is a product
 * p = p_1*p_2*...*p_k, such that each p_i is an odd prime, and p is in
 * the allowed range ((4/3)*2^30 to 2^31), then all operations work and
 * really compute things modulo each p_i simultaneously (through the
 * CRT). In particular, we can compute modulo q = 12289 by using (for
 * instance) p = 2013442049 = 163841*q (value 163841 is itself prime,
 * and 163840 is a multiple of 2048, which is not actually needed if we
 * are only interested in correct computations modulo q).
 */

/*
 * Get v mod p in the 0 to p-1 range; input v must be in the -(p-1) to +(p-1)
 * range.
 */
static inline uint32_t
mp_set(int32_t v, uint32_t p)
{
    uint32_t w = (uint32_t)v;
    return w + (p & tbmask(w));
}

/*
 * Get the signed normalized value of x mod p.
 */
static inline int32_t
mp_norm(uint32_t x, uint32_t p)
{
    uint32_t w = x - (p & tbmask((p >> 1) - x));
    return *(int32_t *)&w;
}

#if 0 /* unused */
/*
 * Compute p0i = -1/p mod 2^32.
 */
static inline uint32_t
mp_ninv32(uint32_t p)
{
    uint32_t y = 2 - p;
    y *= 2 - p * y;
    y *= 2 - p * y;
    y *= 2 - p * y;
    y *= 2 - p * y;
    return -y;
}
#endif

/*
 * Compute R = 2^32 mod p.
 */
static inline uint32_t
mp_R(uint32_t p)
{
    /*
     * Since 2*p < 2^32 < 3*p, we just subtract 2*p from 2^32.
     */
    return -(p << 1);
}

/*
 * Compute R/2 = 2^31 mod p.
 */
static inline uint32_t
mp_hR(uint32_t p)
{
    /*
     * Since p < 2^31 < (3/2)*p, we just subtract p from 2^31.
     */
    return ((uint32_t)1 << 31) - p;
}

/*
 * Addition modulo p.
 */
static inline uint32_t
mp_add(uint32_t a, uint32_t b, uint32_t p)
{
    uint32_t d = a + b - p;
    return d + (p & tbmask(d));
}

/*
 * Subtraction modulo p.
 */
static inline uint32_t
mp_sub(uint32_t a, uint32_t b, uint32_t p)
{
    uint32_t d = a - b;
    return d + (p & tbmask(d));
}

/*
 * Halving modulo p.
 */
static inline uint32_t
mp_half(uint32_t a, uint32_t p)
{
    return (a + (p & -(a & 1))) >> 1;
}

/*
 * Montgomery multiplication modulo p.
 *
 * Reduction computes (a*b + w*p)/(2^32) for some w <= 2^(32-1);
 * then p is conditionally subtracted. This process works as long as:
 *    (a*b + p*(2^32-1))/(2^32) <= 2*p-1
 * which holds if:
 *    a*b <= p*2^32 - 2^32 + p
 * This works if both a and b are proper integers modulo p (in the 0 to p-1
 * range), but also if, for instance, a is an integer modulo p, and b is an
 * arbitrary 32-bit integer.
 */
static inline uint32_t
mp_montymul(uint32_t a, uint32_t b, uint32_t p, uint32_t p0i)
{
    uint64_t z = (uint64_t)a * (uint64_t)b;
    uint32_t w = (uint32_t)z * p0i;
    uint32_t d = (uint32_t)((z + (uint64_t)w * (uint64_t)p) >> 32) - p;
    return d + (p & tbmask(d));
}

/*
 * Compute 2^(31*e) mod p.
 */
static inline uint32_t
mp_Rx31(unsigned e, uint32_t p, uint32_t p0i, uint32_t R2)
{
    /* x <- 2^63 mod p = Montgomery representation of 2^31 */
    uint32_t x = mp_half(R2, p);
    uint32_t d = 1;
    for (;;) {
        if ((e & 1) != 0) {
            d = mp_montymul(d, x, p, p0i);
        }
        e >>= 1;
        if (e == 0) {
            return d;
        }
        x = mp_montymul(x, x, p, p0i);
    }
}

/*
 * Division modulo p (x = dividend, y = divisor).
 * This code uses a constant-time binary GCD, which also works for a
 * non-prime modulus p (contrary to Fermat's Little Theorem). If the
 * divisor is not invertible modulo p, then 0 is returned.
 */
uint32_t mp_div(uint32_t x, uint32_t y, uint32_t p);

/*
 * Compute the roots for NTT; given g (primitive 2048-th root of 1 modulo p),
 * this fills gm[] and igm[] with powers of g and 1/g:
 *    gm[rev(i)] = g^i mod p              (in Montgomery representation)
 *    igm[rev(i)] = (1/2)*(1/g)^i mod p   (in Montgomery representation)
 * rev() is the bit-reversal function over 10 bits. The arrays gm[] and igm[]
 * are filled only up to n = 2^logn values. Roots g and ig must be provided
 * in Montgomery representation.
 */
void mp_mkgmigm(unsigned logn, uint32_t *restrict gm, uint32_t *restrict igm,
    uint32_t g, uint32_t ig, uint32_t p, uint32_t p0i);

/*
 * Like mp_mkgmigm(), but computing only gm[].
 */
void mp_mkgm(unsigned logn, uint32_t *restrict gm,
    uint32_t g, uint32_t p, uint32_t p0i);

/*
 * A variant of mp_mkgm(), specialized for logn = 7, and g being a
 * 256-th root of 1, not a 2048-th root of 1.
 */
void mp_mkgm7(uint32_t *restrict gm, uint32_t g, uint32_t p, uint32_t p0i);

/*
 * Like mp_mkgmigm(), but computing only igm[].
 */
void mp_mkigm(unsigned logn, uint32_t *restrict igm,
    uint32_t ig, uint32_t p, uint32_t p0i);

/*
 * Compute the NTT over a polynomial. The polynomial a[] is modified in-place.
 */
void mp_NTT(unsigned logn, uint32_t *restrict a, const uint32_t *restrict gm,
    uint32_t p, uint32_t p0i);

/*
 * Compute the inverse NTT over a polynomial. The polynomial a[] is modified
 * in-place.
 */
void mp_iNTT(unsigned logn, uint32_t *restrict a, const uint32_t *restrict igm,
    uint32_t p, uint32_t p0i);

/*
 * Precomputed small primes. Enough values are provided to allow
 * computations in RNS representation over big integers up to 10000 bits.
 */
typedef struct {
    uint32_t p;
    uint32_t p0i;
    uint32_t R2;
    uint32_t g;
    uint32_t ig;
    uint32_t s;
} small_prime;
extern const small_prime PRIMES[];

#endif

