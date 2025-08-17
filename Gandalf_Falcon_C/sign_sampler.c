/*
 * Gaussian sampling.
 */

#include "sign_inner.h"

/* Union type to get easier access to values with SIMD intrinsics. */
typedef union {
	fpr f;
} fpr_u;

/* 1/(2*(1.8205^2)) */
#define INV_2SQRSIGMA0   FPR(5435486223186882, -55)

/* For logn = 1 to 10, n = 2^logn:
      q = 12289
      gs_norm = (117/100)*sqrt(q)
      bitsec = max(2, n/4)
      eps = 1/sqrt(bitsec*2^64)
      smoothz2n = sqrt(log(4*n*(1 + 1/eps))/pi)/sqrt(2*pi)
      sigma = smoothz2n*gs_norm
      sigma_min = sigma/gs_norm = smoothz2n
   We store precomputed values for 1/sigma and for sigma_min, indexed
   by logn. */
static const fpr_u INV_SIGMA[] = {
	{ FPR_ZERO },                     /* unused */
	{ FPR(7961475618707097, -60) },   /* 0.0069054793295940881528 */
	{ FPR(7851656902127320, -60) },   /* 0.0068102267767177965681 */
	{ FPR(7746260754658859, -60) },   /* 0.0067188101910722700565 */
	{ FPR(7595833604889141, -60) },   /* 0.0065883354370073655600 */
	{ FPR(7453842886538220, -60) },   /* 0.0064651781207602890978 */
	{ FPR(7319528409832599, -60) },   /* 0.0063486788828078985744 */
	{ FPR(7192222552237877, -60) },   /* 0.0062382586529084365056 */
	{ FPR(7071336252758509, -60) },   /* 0.0061334065020930252290 */
	{ FPR(6956347512113097, -60) },   /* 0.0060336696681577231923 */
	{ FPR(6846791885593314, -60) }    /* 0.0059386453095331150985 */
};
static const fpr_u SIGMA_MIN[] = {
	{ FPR_ZERO },                     /* unused */
	{ FPR(5028307297130123, -52) },   /* 1.1165085072329102589 */
	{ FPR(5098636688852518, -52) },   /* 1.1321247692325272406 */
	{ FPR(5168009084304506, -52) },   /* 1.1475285353733668685 */
	{ FPR(5270355833453349, -52) },   /* 1.1702540788534828940 */
	{ FPR(5370752584786614, -52) },   /* 1.1925466358390344011 */
	{ FPR(5469306724145091, -52) },   /* 1.2144300507766139921 */
	{ FPR(5566116128735780, -52) },   /* 1.2359260567719808790 */
	{ FPR(5661270305715104, -52) },   /* 1.2570545284063214163 */
	{ FPR(5754851361258101, -52) },   /* 1.2778336969128335860 */
	{ FPR(5846934829975396, -52) }    /* 1.2982803343442918540 */
};

#if !FNDSA_ASM_CORTEXM4
/* Distribution for gaussian0() (this is the RCDT table from the
   specification, expressed in base 2^24). */
static const uint32_t GAUSS0[][3] = {
	{ 10745844,  3068844,  3741698 },
	{  5559083,  1580863,  8248194 },
	{  2260429, 13669192,  2736639 },
	{   708981,  4421575, 10046180 },
	{   169348,  7122675,  4136815 },
	{    30538, 13063405,  7650655 },
	{     4132, 14505003,  7826148 },
	{      417, 16768101, 11363290 },
	{       31,  8444042,  8086568 },
	{        1, 12844466,   265321 },
	{        0,  1232676, 13644283 },
	{        0,    38047,  9111839 },
	{        0,      870,  6138264 },
	{        0,       14, 12545723 },
	{        0,        0,  3104126 },
	{        0,        0,    28824 },
	{        0,        0,      198 },
	{        0,        0,        1 }
};
#endif

/* log(2) */
#define LOG2   FPR(6243314768165359, -53)

/* 1/log(2) */
#define INV_LOG2   FPR(6497320848556798, -52)

/* We access the PRNG through macros so that they can be overridden by some
   compatiblity tests with the original Falcon implementation. */
#ifndef prng_init
#if FNDSA_SHAKE256X4
#define prng_init       shake256x4_init
#define prng_next_u8    shake256x4_next_u8
#define prng_next_u64   shake256x4_next_u64
#else
#define prng_init(pc, seed, seed_len)   do { \
		shake_init(pc, 256); \
		shake_inject(pc, seed, seed_len); \
		shake_flip(pc); \
	} while (0)
#define prng_next_u8    shake_next_u8
#define prng_next_u64   shake_next_u64
#endif
#endif

/* see sign_inner.h */
void
sampler_init(sampler_state *ss, unsigned logn,
	const void *seed, size_t seed_len)
{
	prng_init(&ss->pc, seed, seed_len);
	ss->logn = logn;
}

#if FNDSA_ASM_CORTEXM4
int32_t fndsa_gaussian0_helper(uint64_t lo, uint32_t hi);
#endif

static inline int32_t
gaussian0(sampler_state *ss)
{
	/* Get a random 72-bit value, into three 24-bit limbs (v0..v2). */
	uint64_t lo = prng_next_u64(&ss->pc);
	uint32_t hi = prng_next_u8(&ss->pc);
#if FNDSA_ASM_CORTEXM4
	return fndsa_gaussian0_helper(lo, hi);
#else
	uint32_t v0 = (uint32_t)lo & 0xFFFFFF;
	uint32_t v1 = (uint32_t)(lo >> 24) & 0xFFFFFF;
	uint32_t v2 = (uint32_t)(lo >> 48) | (hi << 16);

	/* Sampled value is z such that v0..v2 is lower than the first
	   z elements of the table. */
	int32_t z = 0;
	for (size_t i = 0; i < (sizeof GAUSS0) / sizeof(GAUSS0[0]); i ++) {
		uint32_t cc;
		cc = (v0 - GAUSS0[i][2]) >> 31;
		cc = (v1 - GAUSS0[i][1] - cc) >> 31;
		cc = (v2 - GAUSS0[i][0] - cc) >> 31;
		z += (int32_t)cc;
	}
	return z;
#endif
}

/* ========================= PLAIN IMPLEMENTATION ======================== */

static inline uint64_t
expm_p63(fpr x, fpr ccs)
{
	/* The polynomial approximation of exp(-x) is from FACCT:
	      https://eprint.iacr.org/2018/1234
	   Specifically, the values are extracted from the implementation
	   referenced by the FACCT paper, available at:
	      https://github.com/raykzhao/gaussian  */
	static const uint64_t EXPM_COEFFS[] = {
		0x00000004741183A3,
		0x00000036548CFC06,
		0x0000024FDCBF140A,
		0x0000171D939DE045,
		0x0000D00CF58F6F84,
		0x000680681CF796E3,
		0x002D82D8305B0FEA,
		0x011111110E066FD0,
		0x0555555555070F00,
		0x155555555581FF00,
		0x400000000002B400,
		0x7FFFFFFFFFFF4800,
		0x8000000000000000
	};

	/* TODO: maybe use 64x64->128 multiplications if available? It
	   is a bit tricky to decide, because the plain code is used for
	   unknown architectures, and we do not know if the larger
	   multiplication is constant-time (it often happens that it
	   is not). */

	uint64_t y = EXPM_COEFFS[0];
	uint64_t z = (uint64_t)fpr_trunc(fpr_mul2e(x, 63)) << 1;
	uint32_t z0 = (uint32_t)z, z1 = (uint32_t)(z >> 32);
#if FNDSA_ASM_CORTEXM4
#pragma GCC unroll 12
#endif
	for (size_t i = 1; i < (sizeof EXPM_COEFFS) / sizeof(uint64_t); i ++) {
		uint32_t y0 = (uint32_t)y, y1 = (uint32_t)(y >> 32);
#if FNDSA_ASM_CORTEXM4
		uint32_t tt, r0, r1;
		__asm__(
			"umull	%0, %2, %3, %5\n\t"
			"umull	%0, %1, %3, %6\n\t"
			"umaal	%2, %0, %4, %5\n\t"
			"umaal	%0, %1, %4, %6\n\t"
			: "=&r" (r0), "=&r" (r1), "=&r" (tt)
			: "r" (y0), "r" (y1), "r" (z0), "r" (z1));
		y = EXPM_COEFFS[i] - ((uint64_t)r0 | ((uint64_t)r1 << 32));
#else
		uint64_t f = (uint64_t)z0 * (uint64_t)y0;
		uint64_t a = (uint64_t)z0 * (uint64_t)y1 + (f >> 32);
		uint64_t b = (uint64_t)z1 * (uint64_t)y0;
		uint64_t c = (a >> 32) + (b >> 32)
			+ (((uint64_t)(uint32_t)a
			  + (uint64_t)(uint32_t)b) >> 32)
			+ (uint64_t)z1 * (uint64_t)y1;
		y = EXPM_COEFFS[i] - c;
#endif
	}

	/* The scaling factor must be applied at the end. Since y is now
	   in fixed-point notation, we have to convert the factor to the
	   same format, and we do an extra integer multiplication. */
	uint64_t w = (uint64_t)fpr_trunc(fpr_mul2e(ccs, 63)) << 1;
	uint32_t w0 = (uint32_t)w, w1 = (uint32_t)(w >> 32);
	uint32_t y0 = (uint32_t)y, y1 = (uint32_t)(y >> 32);
#if FNDSA_ASM_CORTEXM4
	uint32_t tt, r0, r1;
	__asm__(
		"umull	%0, %2, %3, %5\n\t"
		"umull	%0, %1, %3, %6\n\t"
		"umaal	%2, %0, %4, %5\n\t"
		"umaal	%0, %1, %4, %6\n\t"
		: "=&r" (r0), "=&r" (r1), "=&r" (tt)
		: "r" (y0), "r" (y1), "r" (w0), "r" (w1));
	y = (uint64_t)r0 | ((uint64_t)r1 << 32);
#else
	uint64_t f = (uint64_t)w0 * (uint64_t)y0;
	uint64_t a = (uint64_t)w0 * (uint64_t)y1 + (f >> 32);
	uint64_t b = (uint64_t)w1 * (uint64_t)y0;
	y = (a >> 32) + (b >> 32)
		+ (((uint64_t)(uint32_t)a + (uint64_t)(uint32_t)b) >> 32)
		+ (uint64_t)w1 * (uint64_t)y1;
#endif
	return y;
}

/* Sample a bit with probability ccs*exp(-x) (for x >= 0). */
static inline int
ber_exp(sampler_state *ss, fpr x, fpr ccs)
{
	/* Reduce x modulo log(2): x = s*log(2) + r, with s an integer,
	   and 0 <= r < log(2). We can use fpr_trunc() because x >= 0
	   (fpr_trunc() is presumably a bit faster than fpr_floor()). */
	int32_t si = (int32_t)fpr_trunc(fpr_mul(x, INV_LOG2));
	fpr r = fpr_sub(x, fpr_mul(fpr_of32(si), LOG2));

	/* If s >= 64, sigma = 1.2, r = 0 and b = 1, then we get s >= 64
	   if the half-Gaussian produced z >= 13, which happens with
	   probability about 2^(-32). When s >= 64, ber_exp() will return
	   true with probability less than 2^(-64), so we can simply
	   saturate s at 63 (the bias introduced here is lower than 2^(-96),
	   and would require about 2^192 samplings to be detectable, which
	   is way beyond the formal bound of 2^64 signatures with the
	   same key. */
	uint32_t s = (uint32_t)si;
	s |= (uint32_t)(63 - s) >> 26;
	s &= 63;

	/* Compute ccs*exp(-x). Since x = s*log(2) + r, we compute
	   ccs*exp(-r)/2^s. We know that 0 <= r < log(2), so we can
	   use expm_p63(), which yields a result scaled by 63 bits. We
	   scale it up 1 bit further, then right-shift by s bits.

	   We subtract 1 to make sure that the value fits on 64 bits
	   (i.e. if r = 0 then we may get 2^64 and we prefer 2^64-1
	   in that case, to avoid the overflow). The bias is negligible
	   since expm_p63() has precision only 51 bits or so. */
	uint64_t z = fpr_ursh((expm_p63(r, ccs) << 1) - 1, s);

	/* Sample a bit. We lazily compare the value z with a uniform 64-bit
	   integer, consuming only as many bytes as necessary. Since the PRNG
	   is cryptographically strong, we leak no information from the
	   conditional jumps below. */
	for (int i = 56; i >= 0; i -= 8) {
		unsigned w = prng_next_u8(&ss->pc);
		unsigned bz = (unsigned)(z >> i) & 0xFF;
		if (w != bz) {
			return w < bz;
		}
	}
	return 0;
}

/* see sign_inner.h */
int32_t
sampler_next(sampler_state *ss, fpr mu, fpr isigma)
{
	/* Split center mu into s + r, for an integer s, and 0 <= r < 1. */
	int64_t s = fpr_floor(mu);
	fpr r = fpr_sub(mu, fpr_of32((int32_t)s));

	/* dss = 1/(2*sigma^2) = 0.5*(isigma^2)  */
	fpr dss = fpr_half(fpr_sqr(isigma));

	/* css = sigma_min / sigma = sigma_min * isigma  */
	fpr ccs = fpr_mul(isigma, SIGMA_MIN[ss->logn].f);

	/* We sample on centre r. */
	for (;;) {
		/* Sample z for a Gaussian distribution (non-negative only),
		   then get a random bit b to turn the sampling into a
		   bimodal distribution (we use z+1 if b = 1, or -z
		   otherwise). */
		int32_t z0 = gaussian0(ss);
		int32_t b = prng_next_u8(&ss->pc) & 1;
		int32_t z = b + ((b << 1) - 1) * z0;

		/* Rejection sampling. We want a Gaussian centred on r,
		   but we sampled against a bimodal distribution (with
		   "centres" at 0 and 1). However, we know that z is
		   always in the range where our sampling distribution is
		   greater than the Gaussian distribution, so rejection works.

		   We got z from distribution:
		      G(z) = exp(-((z-b)^2)/(2*sigma0^2))
		   We target distribution:
		      S(z) = exp(-((z-r)^2)/(2*signa^2))
		   Rejection sampling works by keeping the value z with
		   probability S(z)/G(z), and starting again otherwise.
		   This requires S(z) <= G(z), which is the case here.
		   Thus, we simply need to keep our z with probability:
		      P = exp(-x)
		   where:
		      x = ((z-r)^2)/(2*sigma^2) - ((z-b)^2)/(2*sigma0^2)
		   Here, we scale up the Bernouilli distribution, which
		   makes rejection more probable, but also makes the
		   rejection rate sufficiently decorrelated from the Gaussian
		   centre and standard deviation, so that measurement of the
		   rejection rate does not leak enough usable information
		   to attackers (which is how the implementation can claim
		   to be "constant-time").  */
		fpr x = fpr_mul(fpr_sqr(fpr_sub(fpr_of32(z), r)), dss);
		x = fpr_sub(x, fpr_mul(fpr_of32(z0 * z0), INV_2SQRSIGMA0));
		if (ber_exp(ss, x, ccs)) {
			return (int32_t)s + z;
		}
	}
}

#if !FNDSA_ASM_CORTEXM4
TARGET_SSE2 TARGET_NEON static
#endif
void
ffsamp_fft_deepest(sampler_state *ss, fpr *tmp)
{
	fpr *t0 = tmp;
	fpr *t1 = tmp + 2;
	fpr *g01 = tmp + 4;
	fpr *g00 = tmp + 6;
	fpr *g11 = tmp + 7;
	/* Decompose G into LDL. g00 and g11 are self-adjoint,
	   thus only one (real) coefficient each. */
	fpr g00_re = g00[0];
	fpr g01_re = g01[0], g01_im = g01[1];
	fpr g11_re = g11[0];
	fpr inv_g00_re = fpr_inv(g00_re);
	fpr mu_re = fpr_mul(g01_re, inv_g00_re);
	fpr mu_im = fpr_mul(g01_im, inv_g00_re);
	fpr zo_re = fpr_add(
		fpr_mul(mu_re, g01_re),
		fpr_mul(mu_im, g01_im));
	fpr d00_re = g00_re;
	fpr l01_re = mu_re;
	fpr l01_im = fpr_neg(mu_im);
	fpr d11_re = fpr_sub(g11_re, zo_re);

	/* No split on d00 and d11, since they are one-coeff each. */

	/* The half-size Gram matrices for the recursive LDL tree
	   exploration are now:
	     - left sub-tree:   d00_re, zero, d00_re
	     - right sub-tree:  d11_re, zero, d11_re
	   t1 split is trivial. */
	fpr w0 = t1[0];
	fpr w1 = t1[1];
	fpr leaf = fpr_mul(fpr_sqrt(d11_re), INV_SIGMA[ss->logn].f);
	fpr y0 = fpr_of32(sampler_next(ss, w0, leaf));
	fpr y1 = fpr_of32(sampler_next(ss, w1, leaf));

	/* Merge is trivial, since logn = 1. */

	/* At this point:
	     t0 and t1 are unmodified; t1 is also [w0, w1]
	     l10 is in [l10_re, l10_im]
	     z1 is [y0, y1]
	   Compute tb0 = t0 + (t1 - z1)*l10  (into [x0, x1]).
	   z1 is moved into t1. */
	fpr a_re = fpr_sub(w0, y0);
	fpr a_im = fpr_sub(w1, y1);
	fpr b_re, b_im;
	FPC_MUL(b_re, b_im, a_re, a_im, l01_re, l01_im);
	fpr x0 = fpr_add(t0[0], b_re);
	fpr x1 = fpr_add(t0[1], b_im);
	t1[0] = y0;
	t1[1] = y1;

	/* Second recursive invocation, on the split tb0, using
	   the left sub-tree. tb0 is [x0, x1], and the split is
	   trivial since logn = 1. */
	leaf = fpr_mul(fpr_sqrt(d00_re), INV_SIGMA[ss->logn].f);
	t0[0] = fpr_of32(sampler_next(ss, x0, leaf));
	t0[1] = fpr_of32(sampler_next(ss, x1, leaf));
	return;
}

#if FNDSA_ASM_CORTEXM4
#define ffsamp_fft_inner   fndsa_ffsamp_fft_inner
void ffsamp_fft_inner(sampler_state *ss, unsigned logn, fpr *tmp);
#else
TARGET_SSE2 TARGET_NEON
static void
ffsamp_fft_inner(sampler_state *ss, unsigned logn, fpr *tmp)
{
	/* When logn = 1, arrays have length 2; we unroll the last steps
	   in a dedicated function. */
	if (logn == 1) {
		ffsamp_fft_deepest(ss, tmp);
		return;
	}

	/* General case: logn >= 2 */

	/* Layout: we split the tmp[] space into 28 chunks of size qn = n/4.
	   All offsets below are expressed in multiples of qn.

	   Input:
	      0..3     t0
	      4..7     t1
	      8..11    g01
	      12..13   g00 (self-adjoint)
	      14..15   g11 (self-adjoint)
	      16..27   free space

	   All recursive calls make the callee input start at offset 14:
	   the callee receives half the space for half the degree. */
#define qc(off)   (tmp + ((off) << (logn - 2)))

	/* Decompose G into LDL; the decomposed matrix replaces G. */
	fpoly_LDL_fft(logn, qc(12), qc(8), qc(14));

	/* Current layout:
	      0..3     t0
	      4..7     t1
	      8..11    l10
	      12..13   d00 (self-adjoint)
	      14..15   d11 (self-adjoint)
	      16..27   free space  */

	/* Split d11 into the right sub-tree (split yields right_00 and
	   right_01, right_11 is a copy of right_00). */
	fpoly_split_selfadj_fft(logn, qc(20), qc(18), qc(14));
	memcpy(qc(21), qc(20), sizeof(fpr) << (logn - 2));

	/* Current layout:
	      0..3     t0
	      4..7     t1
	      8..11    l10
	      12..13   d00 (self-adjoint)
	      14..17   free space
	      18..19   right_01
	      20       right_00
	      21       right_11
	      22..27   free space  */

	/* Split t1 and make the first recursive call on the two
	   halves, using the right sub-tree, then merge the result
	   into 18..21 */
	fpoly_split_fft(logn, qc(14), qc(16), qc(4));
	ffsamp_fft_inner(ss, logn - 1, qc(14));
	fpoly_merge_fft(logn, qc(18), qc(14), qc(16));

	/* Current layout:
	      0..3     t0
	      4..7     t1
	      8..11    l10
	      12..13   d00 (self-adjoint)
	      14..17   free space
	      18..21   z1
	      22..27   free space  */

	/* Compute tb0 = t0 + (t1 - z1)*l10 (into t0) and move z1 into t1. */
	memcpy(qc(14), qc(4), sizeof(fpr) << logn);
	fpoly_sub(logn, qc(14), qc(18));
	memcpy(qc(4), qc(18), sizeof(fpr) << logn);
	fpoly_mul_fft(logn, qc(14), qc(8));
	fpoly_add(logn, qc(0), qc(14));

	/* Current layout:
	      0..3     tb0
	      4..7     z1
	      8..11    free space
	      12..13   d00 (self-adjoint)
	      14..27   free space  */

	/* Split d00 to obtain the left-subtree. */
	fpoly_split_selfadj_fft(logn, qc(20), qc(18), qc(12));
	memcpy(qc(21), qc(20), sizeof(fpr) << (logn - 2));

	/* Current layout:
	      0..3     tb0
	      4..7     z1
	      8..17    free space
	      18..19   left_01
	      20       left_00
	      21       left_11
	      22..27   free space  */

	/* Split tb0 and perform the second recursive call on the
	   split output; the final merge produces z0, which we write
	   into t0. */
	fpoly_split_fft(logn, qc(14), qc(16), qc(0));
	ffsamp_fft_inner(ss, logn - 1, qc(14));
	fpoly_merge_fft(logn, qc(0), qc(14), qc(16));

#undef qc
}
#endif

/* see sign_inner.h */
void
ffsamp_fft(sampler_state *ss, fpr *tmp)
{
	ffsamp_fft_inner(ss, ss->logn, tmp);
}
