#ifndef NG_FXP_H
#define NG_FXP_H

#include <stdint.h>
#include <memory.h>
#include <math.h>

/* ====================================================================== */
/*
 * Fixed-point numbers.
 *
 * For FFT and other computations with approximations, we use a fixed-point
 * format over 64 bits; the top 32 bits are the integral part, and the low
 * 32 bits are the fractional part.
 */

/*
 * We wrap the type into a struct in order to detect any attempt at using
 * arithmetic operators on values directly. Since all functions are inline,
 * the compiler will be able to remove the wrapper, which will then have
 * no runtime cost.
 */
typedef struct {
    uint64_t v;
} fxr;

#define FXR(x)   { (x) }

static inline void
fxr_of_double(fxr *a, double b){
    int64_t t;
    t = (int64_t)(b * 4294967296.0);
    memmove(a, &t, sizeof(fxr));
}

static inline void
fxr_to_double(double *b, fxr a){
    int64_t t;
    t = *(int64_t*)&a.v;
    *b = t / 4294967296.0;
}

static inline fxr
fxr_of(int32_t j)
{
    fxr x;

    x.v = (uint64_t)j << 32;
    return x;
}

static inline fxr
fxr_of_scaled32(uint64_t t)
{
    fxr x;

    x.v = t;
    return x;
}

static inline fxr
fxr_add(fxr x, fxr y)
{
    x.v += y.v;
    return x;
}

static inline fxr
fxr_sub(fxr x, fxr y)
{
    x.v -= y.v;
    return x;
}

static inline fxr
fxr_double(fxr x)
{
    x.v <<= 1;
    return x;
}

static inline fxr
fxr_neg(fxr x)
{
    x.v = -x.v;
    return x;
}

static inline fxr
fxr_abs(fxr x)
{
    x.v -= (x.v << 1) & (uint64_t)(*(int64_t *)&x.v >> 63);
    return x;
}

static inline fxr
fxr_mul(fxr x, fxr y)
{
#if defined __GNUC__ && defined __SIZEOF_INT128__
    __int128 z;

    z = (__int128)*(int64_t *)&x.v * (__int128)*(int64_t *)&y.v;
    x.v = (uint64_t)(z >> 32);
    return x;
#else
    int32_t xh, yh;
    uint32_t xl, yl;
    uint64_t z0, z1, z2, z3;

    xl = (uint32_t)x.v;
    yl = (uint32_t)y.v;
    xh = (int32_t)(*(int64_t *)&x.v >> 32);
    yh = (int32_t)(*(int64_t *)&y.v >> 32);
    z0 = ((uint64_t)xl * (uint64_t)yl) >> 32;
    z1 = (uint64_t)((int64_t)xl * (int64_t)yh);
    z2 = (uint64_t)((int64_t)yl * (int64_t)xh);
    z3 = (uint64_t)((int64_t)xh * (int64_t)yh) << 32;
    x.v = z0 + z1 + z2 + z3;
    return x;
#endif
}

static inline fxr
fxr_sqr(fxr x)
{
#if defined __GNUC__ && defined __SIZEOF_INT128__
    int64_t t;
    __int128 z;

    t = *(int64_t *)&x.v;
    z = (__int128)t * (__int128)t;
    x.v = (uint64_t)(z >> 32);
    return x;
#else
    int32_t xh;
    uint32_t xl;
    uint64_t z0, z1, z3;

    xl = (uint32_t)x.v;
    xh = (int32_t)(*(int64_t *)&x.v >> 32);
    z0 = ((uint64_t)xl * (uint64_t)xl) >> 32;
    z1 = (uint64_t)((int64_t)xl * (int64_t)xh);
    z3 = (uint64_t)((int64_t)xh * (int64_t)xh) << 32;
    x.v = z0 + (z1 << 1) + z3;
    return x;
#endif
}

static inline int32_t
fxr_round(fxr x)
{
    x.v += 0x80000000ul;
    return (int32_t)(*(int64_t *)&x.v >> 32);
}

static inline fxr
fxr_div2e(fxr x, unsigned n)
{
    x.v += (((uint64_t)1 << n) >> 1);
    x.v = (uint64_t)(*(int64_t *)&x.v >> n);
    return x;
}

static inline fxr
fxr_mul2e(fxr x, unsigned n)
{
    x.v <<= n;
    return x;
}

uint64_t inner_fxr_div(uint64_t x, uint64_t y);

static inline fxr
fxr_inv(fxr x)
{
    x.v = inner_fxr_div((uint64_t)1 << 32, x.v);
    return x;
}

static inline fxr
fxr_div(fxr x, fxr y)
{
    x.v = inner_fxr_div(x.v, y.v);
    return x;
}

static inline int
fxr_lt(fxr x, fxr y)
{
    return *(int64_t *)&x.v < *(int64_t *)&y.v;
}

static const fxr fxr_zero = { 0 };
static const fxr fxr_sqrt2 = { 6074001000ull };

/*
 * A complex value.
 */
typedef struct {
    fxr re, im;
} fxc;

#define FXC(re, im)   { FXR(re), FXR(im) }

static inline fxc
fxc_add(fxc x, fxc y)
{
    x.re = fxr_add(x.re, y.re);
    x.im = fxr_add(x.im, y.im);
    return x;
}

static inline fxc
fxc_sub(fxc x, fxc y)
{
    x.re = fxr_sub(x.re, y.re);
    x.im = fxr_sub(x.im, y.im);
    return x;
}

static inline fxc
fxc_half(fxc x)
{
    x.re = fxr_div2e(x.re, 1);
    x.im = fxr_div2e(x.im, 1);
    return x;
}

static inline fxc
fxc_mul(fxc x, fxc y)
{
    /*
     * We are computing r = (a + i*b)*(c + i*d) with:
     *   z0 = a*c
     *   z1 = b*d
     *   z2 = (a + b)*(c + d)
     *   r = (z0 - z1) + i*(z2 - (z0 + z1))
     * Since the intermediate values are truncated to our precision,
     * the imaginary value of r _may_ be slightly different from
     * a*d + b*c (if we had calculated it directly). For full
     * reproducibility, all implementations should use the formulas
     * above.
     */
    fxr z0 = fxr_mul(x.re, y.re);
    fxr z1 = fxr_mul(x.im, y.im);
    fxr z2 = fxr_mul(fxr_add(x.re, x.im), fxr_add(y.re, y.im));
    fxc z;
    z.re = fxr_sub(z0, z1);
    z.im = fxr_sub(z2, fxr_add(z0, z1));
    return z;
}

static inline fxr
fxc_norm(fxc x)
{
    return fxr_add(fxr_sqr(x.re), fxr_sqr(x.im));
}

static inline fxc
fxc_conj(fxc x)
{
    x.im = fxr_neg(x.im);
    return x;
}

static inline fxc
fxc_div(fxc x, fxc y)
{
    fxr y_norm;
    fxc z;

    y_norm = fxc_norm(y);
    z = fxc_mul(x, fxc_conj(y));
    z.re = fxr_div(z.re, y_norm);
    z.im = fxr_div(z.im, y_norm);

    return z;

}

#endif

