/*
 * Computations modulo q = 12289.
 */

#include "inner.h"

/* Modulus q = 12289 */
#define Q          12289

/* -1/q mod 2^32 */
#define Q1I   4143984639

/* 2^64 mod q */
#define R2          5664

#if FNDSA_ASM_CORTEXM4
/*
 * On the ARM Cortex-M4, we use a relaxed internal representation with
 * values in [0,q]; this makes the ext-to-int operation trivial (internal
 * representation is a superset of external representation) and allows
 * some speed-ups in the NTT.
 */

static inline uint32_t
mq_add(uint32_t x, uint32_t y)
{
	x += y - Q;
	x += Q & (x >> 16);
	return x;
}

static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	x -= y;
	x += Q & (y >> 16);
	return x;
}

static inline uint32_t
mq_half(uint32_t x)
{
	x += Q & -(x & 1);
	return x >> 1;
}

static inline uint32_t
mq_mred(uint32_t x)
{
	uint32_t t;
	__asm__("mul %1, %0, %3\n\tumaal %3, %0, %1, %2"
		: "+r" (x), "=&r" (t)
		: "r" (Q), "r" (Q1I));
	(void)t;
	return x;
}

#else

static inline uint32_t
mq_add(uint32_t x, uint32_t y)
{
	/* -(x+y) in [-q,+q-2] */
	x = Q - (x + y);
	/* normalize to [0,+q-1] */
	x += Q & (x >> 16);
	/* negate to get x+y in [1,q] */
	return Q - x;
}

static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/* -(x-y) in [-(q-1),+q-1] */
	y -= x;
	/* normalize to [0,+q-1] */
	y += Q & (y >> 16);
	/* negate to get x-y in [1,q] */
	return Q - y;
}

static inline uint32_t
mq_half(uint32_t x)
{
	x += Q & -(x & 1);
	return x >> 1;
}

static inline uint32_t
mq_mred(uint32_t x)
{
	x *= Q1I;
	x = (x >> 16) * Q;
	return (x >> 16) + 1;
}

#endif

static inline uint32_t
mq_mmul(uint32_t x, uint32_t y) {
	return mq_mred(x * y);
}

static uint32_t
mq_div(uint32_t x, uint32_t y)
{
	/* Convert y to Montgomery representation. */
	y = mq_mmul(y, R2);

	/* 1/y = y^(q-2), with a custom addition chain. */
	uint32_t y2 = mq_mmul(y, y);
	uint32_t y3 = mq_mmul(y2, y);
	uint32_t y5 = mq_mmul(y3, y2);
	uint32_t y10 = mq_mmul(y5, y5);
	uint32_t y20 = mq_mmul(y10, y10);
	uint32_t y40 = mq_mmul(y20, y20);
	uint32_t y80 = mq_mmul(y40, y40);
	uint32_t y160 = mq_mmul(y80, y80);
	uint32_t y163 = mq_mmul(y160, y3);
	uint32_t y323 = mq_mmul(y163, y160);
	uint32_t y646 = mq_mmul(y323, y323);
	uint32_t y1292 = mq_mmul(y646, y646);
	uint32_t y1455 = mq_mmul(y1292, y163);
	uint32_t y2910 = mq_mmul(y1455, y1455);
	uint32_t y5820 = mq_mmul(y2910, y2910);
	uint32_t y6143 = mq_mmul(y5820, y323);
	uint32_t y12286 = mq_mmul(y6143, y6143);
	uint32_t iy = mq_mmul(y12286, y);

	/* Multiply by the dividend (x) to get x/y. Since iy is in
	   Montgomery representation but x is in normal representation,
	   the result is in normal representation. */
	return mq_mmul(x, iy);
}

#if FNDSA_AVX2

TARGET_AVX2
static inline __m256i
mq_add_x16(__m256i x, __m256i y)
{
	__m256i qq = _mm256_set1_epi16(Q);
	__m256i a = _mm256_sub_epi16(qq, _mm256_add_epi16(x, y));
	__m256i b = _mm256_add_epi16(a,
		_mm256_and_si256(qq, _mm256_srai_epi16(a, 15)));
	return _mm256_sub_epi16(qq, b);
}

TARGET_AVX2
static inline __m256i
mq_sub_x16(__m256i x, __m256i y)
{
	__m256i qq = _mm256_set1_epi16(Q);
	__m256i a = _mm256_sub_epi16(y, x);
	__m256i b = _mm256_add_epi16(a,
		_mm256_and_si256(qq, _mm256_srai_epi16(a, 15)));
	return _mm256_sub_epi16(qq, b);
}

TARGET_AVX2
static inline __m256i
mq_half_x16(__m256i x)
{
	__m256i qq = _mm256_set1_epi16(Q);
	__m256i y = _mm256_and_si256(x, _mm256_set1_epi16(1));
	y = _mm256_sub_epi16(_mm256_setzero_si256(), y);
	y = _mm256_and_si256(y, qq);
	y = _mm256_add_epi16(x, y);
	return _mm256_srli_epi16(y, 1);
}

/* Q1I split into low and high parts (16 bits each), converted to
   signed 16-bit representation. */
#define Q1Ilo   12287
#define Q1Ihi   -2304

TARGET_AVX2
static inline __m256i
mq_mred_x16(__m256i lo, __m256i hi)
{
	__m256i qx16 = _mm256_set1_epi16(Q);
	__m256i q1ilox16 = _mm256_set1_epi16(Q1Ilo);
	__m256i q1ihix16 = _mm256_set1_epi16(Q1Ihi);

	/* x <- (x * Q1I) >> 16
	   32-bit input is split into its low and high halves. Q1I is
	   itself a 32-bit constant. Product is computed modulo 2^32,
	   and we are interested only in its high 16 bits. */
	__m256i x = _mm256_add_epi16(
		_mm256_add_epi16(
			_mm256_mulhi_epu16(lo, q1ilox16),
			_mm256_mullo_epi16(lo, q1ihix16)),
		_mm256_mullo_epi16(hi, q1ilox16));

	/* x <- (x * Q) >> 16
	   x and Q both fit on 16 bits each. */
	x = _mm256_mulhi_epu16(x, qx16);

	/* Result is x + 1. */
	return _mm256_add_epi16(x, _mm256_set1_epi16(1));
}

TARGET_AVX2
static inline __m256i
mq_mmul_x16(__m256i x, __m256i y)
{
	return mq_mred_x16(_mm256_mullo_epi16(x, y), _mm256_mulhi_epu16(x, y));
}

TARGET_AVX2
static __m256i
mq_div_x16(__m256i x, __m256i y)
{
	/* Convert y to Montgomery representation. */
	y = mq_mmul_x16(y, _mm256_set1_epi16(R2));

	/* 1/y = y^(q-2), with a custom addition chain. */
	__m256i y2 = mq_mmul_x16(y, y);
	__m256i y3 = mq_mmul_x16(y2, y);
	__m256i y5 = mq_mmul_x16(y3, y2);
	__m256i y10 = mq_mmul_x16(y5, y5);
	__m256i y20 = mq_mmul_x16(y10, y10);
	__m256i y40 = mq_mmul_x16(y20, y20);
	__m256i y80 = mq_mmul_x16(y40, y40);
	__m256i y160 = mq_mmul_x16(y80, y80);
	__m256i y163 = mq_mmul_x16(y160, y3);
	__m256i y323 = mq_mmul_x16(y163, y160);
	__m256i y646 = mq_mmul_x16(y323, y323);
	__m256i y1292 = mq_mmul_x16(y646, y646);
	__m256i y1455 = mq_mmul_x16(y1292, y163);
	__m256i y2910 = mq_mmul_x16(y1455, y1455);
	__m256i y5820 = mq_mmul_x16(y2910, y2910);
	__m256i y6143 = mq_mmul_x16(y5820, y323);
	__m256i y12286 = mq_mmul_x16(y6143, y6143);
	__m256i iy = mq_mmul_x16(y12286, y);

	/* Multiply by the dividend (x) to get x/y. Since iy is in
	   Montgomery representation but x is in normal representation,
	   the result is in normal representation. */
	return mq_mmul_x16(x, iy);
}

#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_small_to_int(unsigned logn, const int8_t *f, uint16_t *d)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = -(uint32_t)f[i];
		d[i] = (uint16_t)(Q - (x + (Q & (x >> 16))));
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_small_to_int(unsigned logn, const int8_t *f, uint16_t *d)
{
	if (logn >= 4) {
		const __m128i *fp = (const __m128i *)f;
		__m128i *dp = (__m128i *)d;
		__m128i qq = _mm_set1_epi16(Q);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m128i x = _mm_loadu_si128(fp + i);
			x = _mm_sub_epi8(_mm_setzero_si128(), x);
			__m128i x0 = _mm_cvtepi8_epi16(x);
			__m128i x1 = _mm_cvtepi8_epi16(_mm_bsrli_si128(x, 8));
			x0 = _mm_add_epi16(x0,
				_mm_and_si128(_mm_srai_epi16(x0, 15), qq));
			x1 = _mm_add_epi16(x1,
				_mm_and_si128(_mm_srai_epi16(x1, 15), qq));
			x0 = _mm_sub_epi16(qq, x0);
			x1 = _mm_sub_epi16(qq, x1);
			_mm_storeu_si128(dp + (i << 1) + 0, x0);
			_mm_storeu_si128(dp + (i << 1) + 1, x1);
		}
	} else {
		size_t n = (size_t)1 << logn;
		for (size_t i = 0; i < n; i ++) {
			uint32_t x = -(uint32_t)f[i];
			d[i] = (uint16_t)(Q - (x + (Q & (x >> 16))));
		}
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_signed_to_int(unsigned logn, uint16_t *d)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = -(uint32_t)*(int16_t *)&d[i];
		d[i] = (uint16_t)(Q - (x + (Q & (x >> 16))));
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_signed_to_int(unsigned logn, uint16_t *d)
{
	if (logn >= 4) {
		__m256i *dp = (__m256i *)d;
		__m256i qq = _mm256_set1_epi16(Q);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i y = _mm256_loadu_si256(dp + i);
			y = _mm256_sub_epi16(_mm256_setzero_si256(), y);
			y = _mm256_add_epi16(y,
				_mm256_and_si256(qq, _mm256_srai_epi16(y, 15)));
			y = _mm256_sub_epi16(qq, y);
			_mm256_storeu_si256(dp + i, y);
		}
	} else {
		size_t n = (size_t)1 << logn;
		for (size_t i = 0; i < n; i ++) {
			uint32_t x = -(uint32_t)*(int16_t *)&d[i];
			d[i] = (uint16_t)(Q - (x + (Q & (x >> 16))));
		}
	}
}
#endif

/* see inner.h */
int
mqpoly_int_to_small(unsigned logn, const uint16_t *d, int8_t *f)
{
	size_t n = (size_t)1 << logn;
	uint32_t ov = 0;
	for (size_t i = 0; i < n; i ++) {
		/* We add 128. If the value is in-range, we get an integer
		   in [1,255]; otherwise, we get an integer in [256,q]. */
		uint32_t x = mq_add(d[i], 128);
		ov |= x;
#if FNDSA_ASM_CORTEXM4
		/* In the relaxed representation, we must also reject a
		   zero here. */
		ov |= x - 1;
#endif
		x -= 128;
		f[i] = (int8_t)*(int32_t *)&x;
	}
	return (ov >> 8) == 0;
}

#if FNDSA_AVX2
TARGET_AVX2
int
avx2_mqpoly_int_to_small(unsigned logn, const uint16_t *d, int8_t *f)
{
	if (logn >= 4) {
		const __m256i *dp = (const __m256i *)d;
		__m128i *fp = (__m128i *)f;
		__m256i y128 = _mm256_set1_epi16(128);
		__m256i sm = _mm256_setr_epi8(
			0, 2, 4, 6, 8, 10, 12, 14,
			-1, -1, -1, -1, -1, -1, -1, -1,
			0, 2, 4, 6, 8, 10, 12, 14,
			-1, -1, -1, -1, -1, -1, -1, -1);
		__m256i ov = _mm256_setzero_si256();
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i y = _mm256_loadu_si256(dp + i);
			y = mq_add_x16(y, y128);
			ov = _mm256_or_si256(ov, y);
			y = _mm256_sub_epi16(y, y128);
			y = _mm256_shuffle_epi8(y, sm);
			y = _mm256_permute4x64_epi64(y, 0x88);
			_mm_storeu_si128(fp + i, _mm256_castsi256_si128(y));
		}
		uint32_t r = (uint32_t)_mm256_movemask_epi8(ov);
		return (r & 0xAAAAAAAA) == 0;
	} else {
		size_t n = (size_t)1 << logn;
		uint32_t ov = 0;
		for (size_t i = 0; i < n; i ++) {
			/* We add 128. If the value is in-range, we get an
			   integer in [1,255]; otherwise, we get an integer
			   in [256,q]. */
			uint32_t x = mq_add(d[i], 128);
			ov |= x;
			x -= 128;
			f[i] = (int8_t)*(int32_t *)&x;
		}
		return (ov >> 8) == 0;
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_ext_to_int(unsigned logn, uint16_t *d)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = d[i];
		x += Q & ((x - 1) >> 16);
		d[i] = (uint16_t)x;
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_ext_to_int(unsigned logn, uint16_t *d)
{
	if (logn >= 4) {
		__m256i *dp = (__m256i *)d;
		__m256i qq = _mm256_set1_epi16(Q);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i y = _mm256_loadu_si256(dp + i);
			y = _mm256_add_epi16(y, _mm256_and_si256(qq,
				_mm256_cmpeq_epi16(y, _mm256_setzero_si256())));
			_mm256_storeu_si256(dp + i, y);
		}
	} else {
		size_t n = (size_t)1 << logn;
		for (size_t i = 0; i < n; i ++) {
			uint32_t x = d[i];
			x += Q & ((x - 1) >> 16);
			d[i] = (uint16_t)x;
		}
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_int_to_ext(unsigned logn, uint16_t *d)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = (uint32_t)d[i] - Q;
		x += Q & (x >> 16);
		d[i] = (uint16_t)x;
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_int_to_ext(unsigned logn, uint16_t *d)
{
	if (logn >= 4) {
		__m256i *dp = (__m256i *)d;
		__m256i qq = _mm256_set1_epi16(Q);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i y = _mm256_loadu_si256(dp + i);
			y = _mm256_sub_epi16(y, _mm256_and_si256(qq,
				_mm256_cmpeq_epi16(y, qq)));
			_mm256_storeu_si256(dp + i, y);
		}
	} else {
		size_t n = (size_t)1 << logn;
		for (size_t i = 0; i < n; i ++) {
			uint32_t x = (uint32_t)d[i] - Q;
			x += Q & (x >> 16);
			d[i] = (uint16_t)x;
		}
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_int_to_ntt(unsigned logn, uint16_t *d)
{
	if (logn == 0) {
		return;
	}
	size_t t = (size_t)1 << logn;
	for (unsigned lm = 0; lm < logn; lm ++) {
		size_t m = (size_t)1 << lm;
		size_t ht = t >> 1;
		size_t j0 = 0;
		for (size_t i = 0; i < m; i ++) {
			uint32_t s = mq_GM[i + m];
			for (size_t j = 0; j < ht; j ++) {
				size_t j1 = j0 + j;
				size_t j2 = j1 + ht;
				uint32_t x1 = d[j1];
				uint32_t x2 = mq_mmul(d[j2], s);
				d[j1] = (uint16_t)mq_add(x1, x2);
				d[j2] = (uint16_t)mq_sub(x1, x2);
			}
			j0 += t;
		}
		t = ht;
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
static inline void
avx2_NTT32(__m256i *a0, __m256i *a1, size_t k)
{
	__m256i ya0, ya1, yt1, yt2, yt3, yt4, yg1, yg2, yg3, ysk;
	uint16_t g1_0, g1_1;

	ya0 = *a0;
	ya1 = *a1;

	/* t = 32, m = 1 */
	yt1 = ya0;
	yt2 = mq_mmul_x16(ya1, _mm256_set1_epi16(mq_GM[k]));
	ya0 = mq_add_x16(yt1, yt2);
	ya1 = mq_sub_x16(yt1, yt2);

	/* ya0:  0  1  2  3  4  5  6  7 |  8  9 10 11 12 13 14 15 */
	/* ya1: 16 17 18 19 20 21 22 23 | 24 25 26 27 28 29 30 31 */

	/* t = 16, m = 2 */
	yt1 = _mm256_permute2x128_si256(ya0, ya1, 0x20);
	yt2 = _mm256_permute2x128_si256(ya0, ya1, 0x31);
	g1_0 = mq_GM[(k << 1) + 0];
	g1_1 = mq_GM[(k << 1) + 1];
	yg1 = _mm256_setr_epi16(
		g1_0, g1_0, g1_0, g1_0, g1_0, g1_0, g1_0, g1_0,
		g1_1, g1_1, g1_1, g1_1, g1_1, g1_1, g1_1, g1_1);
	yt2 = mq_mmul_x16(yt2, yg1);
	ya0 = mq_add_x16(yt1, yt2);
	ya1 = mq_sub_x16(yt1, yt2);

	/* ya0:  0  1  2  3  4  5  6  7 | 16 17 18 19 20 21 22 23 */
	/* ya1:  8  9 10 11 12 13 14 15 | 24 25 26 27 28 29 30 31 */

	/* t = 8, m = 4 */
	yt1 = _mm256_unpacklo_epi64(ya0, ya1);
	yt2 = _mm256_unpackhi_epi64(ya0, ya1);
	yg2 = _mm256_setr_epi64x(
		mq_GM[(k << 2) + 0], mq_GM[(k << 2) + 1],
		mq_GM[(k << 2) + 2], mq_GM[(k << 2) + 3]);
	yg2 = _mm256_or_si256(yg2, _mm256_slli_epi64(yg2, 32));
	yg2 = _mm256_or_si256(yg2, _mm256_slli_epi32(yg2, 16));
	yt2 = mq_mmul_x16(yt2, yg2);
	ya0 = mq_add_x16(yt1, yt2);
	ya1 = mq_sub_x16(yt1, yt2);

	/* ya0:  0  1  2  3  8  9 10 11 | 16 17 18 19 24 25 26 27 */
	/* ya1:  4  5  6  7 12 13 14 15 | 20 21 22 23 28 29 30 31 */

	/* t = 4, m = 8 */
	yt3 = _mm256_shuffle_epi32(ya0, 0xD8);
	yt4 = _mm256_shuffle_epi32(ya1, 0xD8);
	yt1 = _mm256_unpacklo_epi32(yt3, yt4);
	yt2 = _mm256_unpackhi_epi32(yt3, yt4);
	yg3 = _mm256_cvtepi16_epi32(
		_mm_loadu_si128((const __m128i *)mq_GM + k));
	yg3 = _mm256_or_si256(yg3, _mm256_slli_epi32(yg3, 16));
	yt2 = mq_mmul_x16(yt2, yg3);
	ya0 = mq_add_x16(yt1, yt2);
	ya1 = mq_sub_x16(yt1, yt2);

	/* ya0:  0  1  4  5  8  9 12 13 | 16 17 20 21 24 25 28 29 */
	/* ya1:  2  3  6  7 10 11 14 15 | 18 19 22 23 26 27 30 31 */

	/* t = 2, m = 16 */
	ysk = _mm256_setr_epi8(
		0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15,
		0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
	yt3 = _mm256_shuffle_epi8(ya0, ysk);
	yt4 = _mm256_shuffle_epi8(ya1, ysk);
	yt1 = _mm256_unpacklo_epi16(yt3, yt4);
	yt2 = _mm256_unpackhi_epi16(yt3, yt4);
	yt2 = mq_mmul_x16(yt2,
		_mm256_loadu_si256((const __m256i *)mq_GM + k));
	ya0 = mq_add_x16(yt1, yt2);
	ya1 = mq_sub_x16(yt1, yt2);

	/* ya0:  0  2  4  6  8 10 12 14 | 16 18 20 22 24 26 28 30 */
	/* ya1:  1  3  5  7  9 11 13 15 | 17 19 21 23 25 27 29 31 */
	yt1 = _mm256_unpacklo_epi16(ya0, ya1);
	yt2 = _mm256_unpackhi_epi16(ya0, ya1);
	ya0 = _mm256_permute2x128_si256(yt1, yt2, 0x20);
	ya1 = _mm256_permute2x128_si256(yt1, yt2, 0x31);

	*a0 = ya0;
	*a1 = ya1;
}

TARGET_AVX2
void
avx2_mqpoly_int_to_ntt(unsigned logn, uint16_t *d)
{
	if (logn == 0) {
		return;
	}
	if (logn >= 5) {
		__m256i *dp = (__m256i *)d;
		size_t n = (size_t)1 << logn;
		size_t t = n >> 4;
		for (unsigned lm = 0; lm < (logn - 5); lm ++) {
			size_t m = (size_t)1 << lm;
			size_t ht = t >> 1;
			size_t j0 = 0;
			for (size_t i = 0; i < m; i ++) {
				__m256i ys = _mm256_set1_epi16(mq_GM[i + m]);
				for (size_t j = 0; j < ht; j ++) {
					size_t j1 = j0 + j;
					size_t j2 = j1 + ht;
					__m256i y1, y2;
					y1 = _mm256_loadu_si256(dp + j1);
					y2 = _mm256_loadu_si256(dp + j2);
					y2 = mq_mmul_x16(y2, ys);
					_mm256_storeu_si256(dp + j1,
						mq_add_x16(y1, y2));
					_mm256_storeu_si256(dp + j2,
						mq_sub_x16(y1, y2));
				}
				j0 += t;
			}
			t = ht;
		}
		size_t m = n >> 5;
		for (size_t i = 0; i < m; i ++) {
			__m256i ya0 = _mm256_loadu_si256(dp + (i << 1) + 0);
			__m256i ya1 = _mm256_loadu_si256(dp + (i << 1) + 1);
			avx2_NTT32(&ya0, &ya1, i + m);
			_mm256_storeu_si256(dp + (i << 1) + 0, ya0);
			_mm256_storeu_si256(dp + (i << 1) + 1, ya1);
		}
	} else {
		size_t t = (size_t)1 << logn;
		for (unsigned lm = 0; lm < logn; lm ++) {
			size_t m = (size_t)1 << lm;
			size_t ht = t >> 1;
			size_t j0 = 0;
			for (size_t i = 0; i < m; i ++) {
				uint32_t s = mq_GM[i + m];
				for (size_t j = 0; j < ht; j ++) {
					size_t j1 = j0 + j;
					size_t j2 = j1 + ht;
					uint32_t x1 = d[j1];
					uint32_t x2 = mq_mmul(d[j2], s);
					d[j1] = (uint16_t)mq_add(x1, x2);
					d[j2] = (uint16_t)mq_sub(x1, x2);
				}
				j0 += t;
			}
			t = ht;
		}
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_ntt_to_int(unsigned logn, uint16_t *d)
{
	if (logn == 0) {
		return;
	}
	size_t t = 1;
	for (unsigned lm = 0; lm < logn; lm ++) {
		size_t hm = (size_t)1 << (logn - 1 - lm);
		size_t dt = t << 1;
		size_t j0 = 0;
		for (size_t i = 0; i < hm; i ++) {
			uint32_t s = mq_iGM[i + hm];
			for (size_t j = 0; j < t; j ++) {
				size_t j1 = j0 + j;
				size_t j2 = j1 + t;
				uint32_t x1 = d[j1];
				uint32_t x2 = d[j2];
				d[j1] = (uint16_t)mq_half(mq_add(x1, x2));
				d[j2] = (uint16_t)mq_mmul(mq_sub(x1, x2), s);
			}
			j0 += dt;
		}
		t = dt;
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
static inline void
avx2_iNTT32(__m256i *a0, __m256i *a1, size_t k)
{
	__m256i ya0, ya1, yt1, yt2, yt3, yt4, yig0, yig1, yig2, yig3, ysk;
	uint16_t ig1_0, ig1_1;

	ya0 = *a0;
	ya1 = *a1;

	/* ya0:  0  1  2  3  4  5  6  7 |  8  9 10 11 12 13 14 15 */
	/* ya1: 16 17 18 19 20 21 22 23 | 24 25 26 27 28 29 30 31 */

	yt1 = _mm256_permute2x128_si256(ya0, ya1, 0x20);
	yt2 = _mm256_permute2x128_si256(ya0, ya1, 0x31);

	/* yt1:  0  1  2  3  4  5  6  7 | 16 17 18 19 20 21 22 23 */
	/* yt2:  8  9 10 11 12 13 14 15 | 24 25 26 27 28 29 30 31 */

	ysk = _mm256_setr_epi8(
		0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15,
		0, 1, 4, 5, 8, 9, 12, 13, 2, 3, 6, 7, 10, 11, 14, 15);
	yt3 = _mm256_shuffle_epi8(yt1, ysk);
	yt4 = _mm256_shuffle_epi8(yt2, ysk);

	/* yt3:  0  2  4  6  1  3  5  7 | 16 18 20 22 17 19 21 23 */
	/* yt4:  8 10 12 14  9 11 13 15 | 24 26 28 30 25 27 29 31 */

	yt1 = _mm256_unpacklo_epi64(yt3, yt4);
	yt2 = _mm256_unpackhi_epi64(yt3, yt4);
	ya0 = mq_half_x16(mq_add_x16(yt1, yt2));
	ya1 = mq_mmul_x16(mq_sub_x16(yt1, yt2),
		_mm256_loadu_si256((const __m256i *)mq_iGM + k));

	/* ya0:  0  2  4  6  8 10 12 14 | 16 18 20 22 24 26 28 30 */
	/* ya1:  1  3  5  7  9 11 13 15 | 17 19 21 23 25 27 29 31 */

	yt1 = _mm256_blend_epi16(ya0, _mm256_slli_epi32(ya1, 16), 0xAA);
	yt2 = _mm256_blend_epi16(_mm256_srli_epi32(ya0, 16), ya1, 0xAA);
	yig3 = _mm256_cvtepi16_epi32(
		_mm_loadu_si128((const __m128i *)mq_iGM + k));
	yig3 = _mm256_or_si256(yig3, _mm256_slli_epi32(yig3, 16));
	ya0 = mq_half_x16(mq_add_x16(yt1, yt2));
	ya1 = mq_mmul_x16(mq_sub_x16(yt1, yt2), yig3);

	/* ya0:  0  1  4  5  8  9 12 13 | 16 17 20 21 24 25 28 29 */
	/* ya1:  2  3  6  7 10 11 14 15 | 18 19 22 23 26 27 30 31 */

	yt1 = _mm256_blend_epi16(ya0, _mm256_slli_epi64(ya1, 32), 0xCC);
	yt2 = _mm256_blend_epi16(_mm256_srli_epi64(ya0, 32), ya1, 0xCC);
	yig2 = _mm256_setr_epi64x(
		mq_iGM[(k << 2) + 0], mq_iGM[(k << 2) + 1],
		mq_iGM[(k << 2) + 2], mq_iGM[(k << 2) + 3]);
	yig2 = _mm256_or_si256(yig2, _mm256_slli_epi64(yig2, 32));
	yig2 = _mm256_or_si256(yig2, _mm256_slli_epi32(yig2, 16));
	ya0 = mq_half_x16(mq_add_x16(yt1, yt2));
	ya1 = mq_mmul_x16(mq_sub_x16(yt1, yt2), yig2);

	/* ya0:  0  1  2  3  8  9 10 11 | 16 17 18 19 24 25 26 27 */
	/* ya1:  4  5  6  7 12 13 14 15 | 20 21 22 23 28 29 30 31 */

	yt1 = _mm256_unpacklo_epi64(ya0, ya1);
	yt2 = _mm256_unpackhi_epi64(ya0, ya1);
	ig1_0 = mq_iGM[(k << 1) + 0];
	ig1_1 = mq_iGM[(k << 1) + 1];
	yig1 = _mm256_setr_epi16(
		ig1_0, ig1_0, ig1_0, ig1_0, ig1_0, ig1_0, ig1_0, ig1_0,
		ig1_1, ig1_1, ig1_1, ig1_1, ig1_1, ig1_1, ig1_1, ig1_1);
	ya0 = mq_half_x16(mq_add_x16(yt1, yt2));
	ya1 = mq_mmul_x16(mq_sub_x16(yt1, yt2), yig1);

	/* ya0:  0  1  2  3  4  5  6  7 | 16 17 18 19 20 21 22 23 */
	/* ya1:  8  9 10 11 12 13 14 15 | 24 25 26 27 28 29 30 31 */

	yt1 = _mm256_permute2x128_si256(ya0, ya1, 0x20);
	yt2 = _mm256_permute2x128_si256(ya0, ya1, 0x31);
	yig0 = _mm256_set1_epi16(mq_iGM[k]);
	ya0 = mq_half_x16(mq_add_x16(yt1, yt2));
	ya1 = mq_mmul_x16(mq_sub_x16(yt1, yt2), yig0);

	/* ya0:  0  1  2  3  4  5  6  7 |  8  9 10 11 12 13 14 15 */
	/* ya1: 16 17 18 19 20 21 22 23 | 24 25 26 27 28 29 30 31 */

	*a0 = ya0;
	*a1 = ya1;
}

TARGET_AVX2
void
avx2_mqpoly_ntt_to_int(unsigned logn, uint16_t *d)
{
	if (logn == 0) {
		return;
	}
	if (logn >= 5) {
		__m256i *dp = (__m256i *)d;
		size_t n = (size_t)1 << logn;
		size_t m = n >> 5;
		for (size_t i = 0; i < m; i ++) {
			__m256i ya0 = _mm256_loadu_si256(dp + (i << 1) + 0);
			__m256i ya1 = _mm256_loadu_si256(dp + (i << 1) + 1);
			avx2_iNTT32(&ya0, &ya1, i + m);
			_mm256_storeu_si256(dp + (i << 1) + 0, ya0);
			_mm256_storeu_si256(dp + (i << 1) + 1, ya1);
		}
		size_t t = 2;
		for (unsigned lm = 5; lm < logn; lm ++) {
			size_t hm = (size_t)1 << (logn - 1 - lm);
			size_t dt = t << 1;
			size_t j0 = 0;
			for (size_t i = 0; i < hm; i ++) {
				__m256i ys = _mm256_set1_epi16(mq_iGM[i + hm]);
				for (size_t j = 0; j < t; j ++) {
					size_t j1 = j0 + j;
					size_t j2 = j1 + t;
					__m256i y1, y2;
					y1 = _mm256_loadu_si256(dp + j1);
					y2 = _mm256_loadu_si256(dp + j2);
					_mm256_storeu_si256(dp + j1,
						mq_half_x16(
							mq_add_x16(y1, y2)));
					_mm256_storeu_si256(dp + j2,
						mq_mmul_x16(ys,
							mq_sub_x16(y1, y2)));
				}
				j0 += dt;
			}
			t = dt;
		}
	} else {
		size_t t = 1;
		for (unsigned lm = 0; lm < logn; lm ++) {
			size_t hm = (size_t)1 << (logn - 1 - lm);
			size_t dt = t << 1;
			size_t j0 = 0;
			for (size_t i = 0; i < hm; i ++) {
				uint32_t s = mq_iGM[i + hm];
				for (size_t j = 0; j < t; j ++) {
					size_t j1 = j0 + j;
					size_t j2 = j1 + t;
					uint32_t x1 = d[j1];
					uint32_t x2 = d[j2];
					d[j1] = mq_half(mq_add(x1, x2));
					d[j2] = mq_mmul(mq_sub(x1, x2), s);
				}
				j0 += dt;
			}
			t = dt;
		}
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_mul_ntt(unsigned logn, uint16_t *a, const uint16_t *b)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		a[i] = (uint16_t)mq_mmul(mq_mmul(a[i], b[i]), R2);
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_mul_ntt(unsigned logn, uint16_t *a, const uint16_t *b)
{
	if (logn >= 4) {
		__m256i *ap = (__m256i *)a;
		const __m256i *bp = (const __m256i *)b;
		__m256i yR2 = _mm256_set1_epi16(R2);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i ya = _mm256_loadu_si256(ap + i);
			__m256i yb = _mm256_loadu_si256(bp + i);
			ya = mq_mmul_x16(mq_mmul_x16(ya, yb), yR2);
			_mm256_storeu_si256(ap + i, ya);
		}
	} else {
		size_t n = (size_t)1 << logn;
		for (size_t i = 0; i < n; i ++) {
			a[i] = (uint16_t)mq_mmul(mq_mmul(a[i], b[i]), R2);
		}
	}
}
#endif

/* see inner.h */
int
mqpoly_div_ntt(unsigned logn, uint16_t *a, const uint16_t *b)
{
	size_t n = (size_t)1 << logn;
	uint32_t r = 0xFFFFFFFF;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = b[i];
#if FNDSA_ASM_CORTEXM4
		/* In a relaxed representation, we want to detect both
		   x = 0 and x = q. */
		r &= -x;
#endif
		r &= x - Q;
		a[i] = (uint16_t)mq_div(a[i], x);
	}
	return (r >> 16) != 0;
}

#if FNDSA_AVX2
TARGET_AVX2
int
avx2_mqpoly_div_ntt(unsigned logn, uint16_t *a, const uint16_t *b)
{
	if (logn >= 4) {
		__m256i *ap = (__m256i *)a;
		const __m256i *bp = (const __m256i *)b;
		__m256i qq = _mm256_set1_epi16(Q);
		__m256i ov = _mm256_set1_epi16(-1);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i ya = _mm256_loadu_si256(ap + i);
			__m256i yb = _mm256_loadu_si256(bp + i);
			ya = mq_div_x16(ya, yb);
			_mm256_storeu_si256(ap + i, ya);
			ov = _mm256_and_si256(ov, _mm256_sub_epi16(yb, qq));
		}
		uint32_t r = (uint32_t)_mm256_movemask_epi8(ov);
		return (r & 0xAAAAAAAA) == 0xAAAAAAAA;
	} else {
		size_t n = (size_t)1 << logn;
		uint32_t r = 0xFFFFFFFF;
		for (size_t i = 0; i < n; i ++) {
			uint32_t x = b[i];
			r &= x - Q;
			a[i] = (uint16_t)mq_div(a[i], x);
		}
		return (r >> 16) != 0;
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_sub(unsigned logn, uint16_t *a, const uint16_t *b)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		a[i] = (uint16_t)mq_sub(a[i], b[i]);
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
void
mqpoly_add(unsigned logn, uint16_t *a, const uint16_t *b)
{
	size_t n = (size_t)1 << logn;
	for (size_t i = 0; i < n; i ++) {
		a[i] = (uint16_t)mq_add(a[i], b[i]);
	}
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_sub(unsigned logn, uint16_t *a, const uint16_t *b)
{
	if (logn >= 4) {
		__m256i *ap = (__m256i *)a;
		const __m256i *bp = (const __m256i *)b;
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i ya = _mm256_loadu_si256(ap + i);
			__m256i yb = _mm256_loadu_si256(bp + i);
			ya = mq_sub_x16(ya, yb);
			_mm256_storeu_si256(ap + i, ya);
		}
	} else {
		size_t n = (size_t)1 << logn;
		for (size_t i = 0; i < n; i ++) {
			a[i] = (uint16_t)mq_sub(a[i], b[i]);
		}
	}
}
#endif

/* see inner.h */
int
mqpoly_is_invertible(unsigned logn, const int8_t *f, uint16_t *tmp)
{
	size_t n = (size_t)1 << logn;
	mqpoly_small_to_int(logn, f, tmp);
	mqpoly_int_to_ntt(logn, tmp);
	uint32_t r = 0xFFFFFFFF;
	for (size_t i = 0; i < n; i ++) {
#if FNDSA_ASM_CORTEXM4
		r &= -(uint32_t)tmp[i];
#endif
		r &= (uint32_t)tmp[i] - Q;
	}
	return (r >> 16) != 0;
}

#if FNDSA_AVX2
TARGET_AVX2
int
avx2_mqpoly_is_invertible(unsigned logn, const int8_t *f, uint16_t *tmp)
{
	size_t n = (size_t)1 << logn;
	avx2_mqpoly_small_to_int(logn, f, tmp);
	avx2_mqpoly_int_to_ntt(logn, tmp);
	if (logn >= 4) {
		const __m256i *tp = (const __m256i *)tmp;
		__m256i qq = _mm256_set1_epi16(Q);
		__m256i yr = _mm256_set1_epi16(-1);
		for (size_t i = 0; i < (n >> 4); i ++) {
			__m256i y = _mm256_loadu_si256(tp + i);
			yr = _mm256_and_si256(yr, _mm256_sub_epi16(y, qq));
		}
		uint32_t r = (uint32_t)_mm256_movemask_epi8(yr);
		return (r & 0xAAAAAAAA) == 0xAAAAAAAA;
	} else {
		uint32_t r = 0xFFFFFFFF;
		for (size_t i = 0; i < n; i ++) {
			r &= (uint32_t)tmp[i] - Q;
		}
		return (r >> 16) != 0;
	}
}
#endif

/* see inner.h */
void
mqpoly_div_small(unsigned logn, const int8_t *g, const int8_t *f,
        uint16_t *h, uint16_t *tmp)
{
	mqpoly_small_to_int(logn, f, tmp);
	mqpoly_small_to_int(logn, g, h);
	mqpoly_int_to_ntt(logn, tmp);
	mqpoly_int_to_ntt(logn, h);
	mqpoly_div_ntt(logn, h, tmp);
	mqpoly_ntt_to_int(logn, h);
	mqpoly_int_to_ext(logn, h);
}

#if FNDSA_AVX2
TARGET_AVX2
void
avx2_mqpoly_div_small(unsigned logn, const int8_t *g, const int8_t *f,
        uint16_t *h, uint16_t *tmp)
{
	avx2_mqpoly_small_to_int(logn, f, tmp);
	avx2_mqpoly_small_to_int(logn, g, h);
	avx2_mqpoly_int_to_ntt(logn, tmp);
	avx2_mqpoly_int_to_ntt(logn, h);
	avx2_mqpoly_div_ntt(logn, h, tmp);
	avx2_mqpoly_ntt_to_int(logn, h);
	avx2_mqpoly_int_to_ext(logn, h);
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
uint32_t
mqpoly_sqnorm_int(unsigned logn, const uint16_t *a)
{
	/*
	 * The normalized values are at most (q-1)/2 in absolute value,
	 * thus the squares are at most 37748736. If the sum overflows,
	 * then it must at some point be in [2^31,2^32-1], i.e. with
	 * the highest bit set.
	 */
	size_t n = (size_t)1 << logn;
	uint32_t s = 0;
	uint32_t sat = 0;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = a[i];
		x -= Q & ((((Q - 1) >> 1) - x) >> 16);
		int32_t y = *(int32_t *)&x;
		s += (uint32_t)(y * y);
		sat |= s;
	}
	s |= -(sat >> 31);
	return s;
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
uint32_t
mqpoly_sqnorm_int_to_signed(unsigned logn, uint16_t *a)
{
	/*
	 * The normalized values are at most (q-1)/2 in absolute value,
	 * thus the squares are at most 37748736. If the sum overflows,
	 * then it must at some point be in [2^31,2^32-1], i.e. with
	 * the highest bit set.
	 */
	size_t n = (size_t)1 << logn;
	uint32_t s = 0;
	uint32_t sat = 0;
	for (size_t i = 0; i < n; i ++) {
		uint32_t x = a[i];
		x -= Q & ((((Q - 1) >> 1) - x) >> 16);
		a[i] = (uint16_t)x;
		int32_t y = *(int32_t *)&x;
		s += (uint32_t)(y * y);
		sat |= s;
	}
	s |= -(sat >> 31);
	return s;
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
uint32_t
avx2_mqpoly_sqnorm_ext(unsigned logn, const uint16_t *a)
{
	if (logn >= 4) {
		const __m256i *ap = (const __m256i *)a;
		__m256i ys = _mm256_setzero_si256();
		__m256i ysat = _mm256_setzero_si256();
		__m256i qq = _mm256_set1_epi16(Q);
		__m256i hq = _mm256_set1_epi16((Q - 1) >> 1);
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i y = _mm256_loadu_si256(ap + i);

			/* Normalize to [-q/2,+q/2]. */
			__m256i ym = _mm256_cmpgt_epi16(y, hq);
			y = _mm256_sub_epi16(y, _mm256_and_si256(ym, qq));

			/* Compute and add coefficient squares. */
			__m256i ylo = _mm256_mullo_epi16(y, y);
			__m256i yhi = _mm256_mulhi_epi16(y, y);
			__m256i y0 = _mm256_blend_epi16(
				ylo, _mm256_bslli_epi128(yhi, 2), 0xAA);
			__m256i y1 = _mm256_blend_epi16(
				_mm256_bsrli_epi128(ylo, 2), yhi, 0xAA);
			ys = _mm256_add_epi32(ys, _mm256_add_epi32(y0, y1));

			/* Since normalized values are at most floor(q/2)
			   (i.e. 6144), the addition above added at most
			   2*6144^2 = 75497472, which is (much) lower than
			   2^31. If an overflow occurs at some point, then
			   the corresponding slot must first go through
			   a value with its high bit set. */
			ysat = _mm256_or_si256(ysat, ys);
		}

		/* Finish the addition. We saturate to 2^32-1 if any of
		   the overflow bits was set. */
		ys = _mm256_add_epi32(ys, _mm256_srli_epi64(ys, 32));
		ysat = _mm256_or_si256(ysat, ys);
		ys = _mm256_add_epi32(ys, _mm256_bsrli_epi128(ys, 8));
		ysat = _mm256_or_si256(ysat, ys);
		__m128i xs = _mm_add_epi32(
			_mm256_castsi256_si128(ys),
			_mm256_extracti128_si256(ys, 1));
		uint32_t r = (uint32_t)_mm_cvtsi128_si32(xs);
		uint32_t sat = (uint32_t)_mm256_movemask_epi8(ysat);
		sat = (sat & 0x88888888) | (r & 0x80000000);
		sat |= -sat;
		return r | (uint32_t)(*(int32_t *)&sat >> 31);
	} else {
		size_t n = (size_t)1 << logn;
		uint32_t s = 0;
		uint32_t sat = 0;
		for (size_t i = 0; i < n; i ++) {
			uint32_t x = a[i];
			x -= Q & ((((Q - 1) >> 2) - x) >> 16);
			int32_t y = *(int32_t *)&x;
			s += (uint32_t)(y * y);
			sat |= s;
		}
		s |= -(sat >> 31);
		return s;
	}
}
#endif

#if !FNDSA_ASM_CORTEXM4
/* see inner.h */
uint32_t
mqpoly_sqnorm_signed(unsigned logn, const uint16_t *a)
{
	size_t n = (size_t)1 << logn;
	uint32_t s = 0;
	for (size_t i = 0; i < n; i ++) {
		int32_t y = *(int16_t *)&a[i];
		s += (uint32_t)(y * y);
	}
	return s;
}
#endif

#if FNDSA_AVX2
TARGET_AVX2
uint32_t
avx2_mqpoly_sqnorm_signed(unsigned logn, const uint16_t *a)
{
	if (logn >= 4) {
		const __m256i *ap = (const __m256i *)a;
		__m256i ys = _mm256_setzero_si256();
		for (size_t i = 0; i < (1u << (logn - 4)); i ++) {
			__m256i y = _mm256_loadu_si256(ap + i);
			__m256i ylo = _mm256_mullo_epi16(y, y);
			__m256i yhi = _mm256_mulhi_epi16(y, y);
			__m256i y0 = _mm256_blend_epi16(
				ylo, _mm256_bslli_epi128(yhi, 2), 0xAA);
			__m256i y1 = _mm256_blend_epi16(
				_mm256_bsrli_epi128(ylo, 2), yhi, 0xAA);
			ys = _mm256_add_epi32(ys, _mm256_add_epi32(y0, y1));
		}
		ys = _mm256_add_epi32(ys, _mm256_srli_epi64(ys, 32));
		ys = _mm256_add_epi32(ys, _mm256_bsrli_epi128(ys, 8));
		__m128i xs = _mm_add_epi32(
			_mm256_castsi256_si128(ys),
			_mm256_extracti128_si256(ys, 1));
		return (uint32_t)_mm_cvtsi128_si32(xs);
	} else {
		size_t n = (size_t)1 << logn;
		uint32_t s = 0;
		for (size_t i = 0; i < n; i ++) {
			int32_t y = *(int16_t *)&a[i];
			s += (uint32_t)(y * y);
		}
		return s;
	}
}
#endif

/* see inner.h */
int
mqpoly_sqnorm_is_acceptable(unsigned logn, uint32_t norm)
{
	static const uint32_t SQBETA[] = {
		0,        /* unused */
		101498,
		208714,
		428865,
		892039,
		1852696,
		3842630,
		7959734,
		16468416,
		34034726,
		70265242
	};
	return norm <= SQBETA[logn];
}

/* Constants are public so that they can be shared with alternate
   implementations. */
ALIGN32
const uint16_t mq_GM[] = {
	10952, 11183, 10651,  1669, 12036,  5517, 11593,  9397,  7900,  2739,
	10901,   589,   971,  1704,  4857,  5562,  6241, 10889,  7260,  3046,
	 3102,  8228,   519,  6606, 10000,  5956,  6332, 11479,   918,  6357,
	 7237,   196,  8614,  3587, 11068, 11665,  3165,  1074,  8124,  3246,
	 9490, 10617,   946,  1812,  2862,  6807,  6659,  7117,  8726,  9985,
	   10,  9788,  4473,  8204, 11528,  7220,   657, 11417, 10842,  1827,
	 2845,  7372,  8118, 12120, 11262,  7386,   672,  1521,   734,  8135,
	 7848,  5913, 12199, 10220,  8447,  4800,  6849,  8754, 12187,  3390,
	10989,  5616,  4584,  3792,   618,  7653,  2623,  3907,  3775,  8270,
	 2759, 11676,  1514,  9681,   182,  1180,  2453,  9557,  9954,   256,
	 6264,  1450, 11792, 10012,   203,  6988, 12216,  9655,  5443, 11387,
	 9242,  8739,  8394,  9453,   311,  7013,  7618,  1991, 11971,  3340,
	 4457,  7290,  7841,  3977,  8601, 10525,  4232,  8262,  9581, 11207,
	11931,  1055,  6997, 11064,   208, 11882,  5973,  1724, 10020,   954,
	 8750, 11356, 11685,  8508,  4350,  5786,  5458,  1491,   768,  7005,
	 4930,  8196,  9583,  8249,  1639,  9141,  4387,   219, 11680,  3614,
	12116, 10087,  5450,  1034,  4563, 10273,  3081,  2420,  1684,  4031,
	10170,   306,  2111, 11526,   270,  6207, 10670, 10435, 11721,  4420,
	11376, 10826,  3900,  7730,  4465,  7747,  3540, 11743, 10450,  4012,
	  964, 12057,  4262,   759,  3613,  2088,  5007,  4914,  4011,  3318,
	 5112,  9376,  4397, 10007,  1767,  4164,   878,  4072,   106,  2983,
	 7529, 10732,  9138,  2798,  5855,  4200,  6782,  9535,   588,  2867,
	 9859,  5582,  6867,  6710,  3222,  2794,  9738,   206, 10417,  3663,
	11025,  1528,  8132,  3703,  9062,  4601,  5436,  9451,  8397,  5016,
	   34, 11159,  9371,  2283,  4786, 12259, 10689,  6912,  9827,  3754,
	11782,   224,  5481,  4341, 10318,  2616,  8221,  7251,  5761,  8047,
	12181, 12264,  2763,  5760,  6141, 11321,  5722,  4283, 10712,  9762,
	 4502,  2180, 10873,  5134, 11648,  1786,  4530,  9924,   853,  4180,
	10729,  9197,  3043,  9466,  8115,  4268, 10521,  9604,  4260,  3717,
	 1616,  6291,  7617,  3470,  4828, 11586, 10317,  4095,  9487,  2765,
	 5059,  1740,  6777,  4641,  9748,  9994,   490,   341, 10264,  8748,
	11867,  9688,  7615,  6428,  2831,  3500,  4226,  4847,  4534,  4008,
	11122,  5533,  8350,   795, 11388,  5367,  3593,  7090,  7879,  9220,
	 8366,  1709,  3798, 11120,  7291,  6353, 10034,  4826,  3414,  1473,
	 5704,  6327,  5637,  7108,   640, 11982,    12,  6830,   452,  7387,
	 8918,  8664,  9596,  1311,  8475,   255, 12000,  9605,   225, 11317,
	 9947, 10609,  8712,  6113,  8638,  4958, 10454, 10385,  5769,  8504,
	 2950, 11834,  4612, 11536,  8996,  3903,  9480,   829,  3250, 10538,
	 3623, 11876, 10744, 11590,  2487,  8427,  7036,  2539, 11050,  1420,
	10192,  4635, 10030, 10742, 11709,  9879, 10924,  3439,  7271, 11355,
	 4237,   867,  9373, 11614,   765, 11442,  8079,  8356,  2585, 10953,
	 6577,  5505,  6050, 10731,  7026,  5040,  3812,  2703,  8981,  1510,
	 2385, 11817,  3501,  7979,  8782,   895,  6770,  2705,  5127, 11769,
	  941,  9207,  6692,  7466,  9035,  7667,  4419,  2047,  6765, 10100,
	 9872, 10933,  1414, 10113,  8201, 12253, 10369,   921, 12214,   324,
	 4991,  4000, 11852,  7295, 12204,  2825,  4708,  4731,  6540, 11072,
	  560,  7412,  6155,  2904,  5194, 10988,   251,  9730,  5358,  1923,
	 4248,  9176,   515,   233,  4234,  5304,  3820,  3160,  4680,  9276,
	10410,  1727, 10180, 10094,  6584,  7441, 11798,  1138,  5220,  9401,
	 1634,  4247,  8295,  8406,  5916,     4,  1666,  6075,  4486,  1266,
	 1023, 10819,  7623,  6885,  2252, 11900, 12024, 10976, 10500,  3796,
	 1733,  5294,  2930,  4547,   823, 11683, 10518,  1752,  7417,  4334,
	 6144,  6884,  2573,  4123,  6797, 11928,  9421,  2067,  6820,  2489,
	 1664,  9033,  9425,  8440,  3633,  9375,  8555,  4825,  7457,  6619,
	 6426,  7632,  1503,  1372, 11142,   531,  3742,  7921,  9866,  7518,
	 7712, 10433,  4985,   585,  6622,   395,  7745, 10782,  9746,   663,
	11926,  8450,    70,  7071,  6733,  8272,  6962,  1384,  4599,  6185,
	 2160,   500,  7626,  2448,  7670, 11106,  5100,  2546,  4704, 10647,
	 5138,  7789,  5780,  4524, 11659, 10095,  9973,  9022, 11076, 12122,
	11575, 11441,  3189,  2445,  7510,  1966,  4326,  4415,  6072,  2771,
	 1847,  8734,  7024,  7998, 10598,  6322,  1274,  8260,  4882,  5454,
	 8233,  1792,  6981, 10150,  8810,  8639,  1421, 12049, 11778,  6140,
	 1234,  5975,  3249, 12017,  9602,  4726,  2177, 12224,  4170,  1648,
	10063, 11091,  6621,  1874,  5731,  3261, 11051, 12230,  5046,  8678,
	 5622,  4715,  9783,  7385, 12112,  3714,  1456,  9440,  4944, 12068,
	 8695,  6678, 12094,  5758,  8061, 10400,  5872,  3635,  1339, 10437,
	 5376, 12168,  9932,  8216,  5636,  8587, 11473,  2542,  6131,  1533,
	 8026,   720, 11078,  9164,  1283,  7238,  7363, 10466,  9278,  4651,
	11788,  3639,  9745,  2142,  2488,  6948,  1890,  6582,   956, 11600,
	 8313,  6362,  5898,  2048,  2722,  4954,  6677,  5073,   202,  8467,
	11705,  3506,  6748, 10665,  5256,  5313,   713,  2327, 10471,  9820,
	 3499, 10937, 11206,  4187,  6201,  8604,    80,  4570,  6146,  3926,
	  742,  8592,  3547,  1390,  2521,  7297,  4118,  4822, 10607,  5300,
	 4116,  7780,  7568,  2207, 11202, 10103, 10265,  7269,  6721,  1442,
	11474,  1063,  3441, 10696,  7768,  1343,  1989,  7629,  1185,  4712,
	 9623, 10534,   238,  4379,  4152,  3692,  8924, 12079,  1089, 11517,
	 7344,  1700,  8740,  1568,  1500,  5809, 10781,  6023,  8391,  1601,
	 3460,  7173, 11533, 12114,  7052,  3453,  6120,  5513,  3187,  5403,
	 1250,  6889,  6936,  2971,  2377, 11360,  7802,   213,  7132,  8023,
	 5971,  4682,  1369,  2934,  9012,  4817,  7649,  5298, 12202,  5783,
	 5242,  1441, 11312,  7170,  4163, 12001,  9218,  7368, 10774,  4087,
	 4964,  7066, 10835, 12180, 10572,  7909,  6791,  8513,  3430,  2387,
	10403, 12080,  9335,  6371,  4149,  8129,  7528, 12211,  5004,  9351,
	 7160,  3478,  4120,  1864,  9294,  5565,  5982,   702,   573,   474,
	 5997,  3095,  9406, 11963,  2008,  4106,  1881,  7604,  8793,  9204,
	11609, 10311,  3061,  7422,  2592,   600,  4480, 10140,    84, 10943,
	 3164,  2553,   981, 11492,  5727,  9177, 10169,  1785, 10266,  5790,
	 1575,  5485,  8184,   529, 11828,  5924, 11310, 10128, 11733, 11250,
	 3516, 10372,  8361,  9104,  7706,  7018,  1527,  2743,  4915,  5803,
	10461,    32,   783,  9398,  1474,  7396,  5120,  9833,    96,  5484,
	 3616,  9940,  9899,  7867,  8765,  1460,  8229,  7708,  2734, 11784,
	 1741,  5751,  5081,  6069,  4166,  7564,  5355,  6360,  7397,  9336,
	 5806,  2937,  9172,  1668,  5483,  1383,    26, 10702,  2106,  6632,
	 1422, 10570,  4406,  8985, 12218,  6697,    29,  6265, 10523,  6646,
	11311,  8649,  6587,  3004,  9977,  3106,  1800,  4513,  6355,  2040,
	10488,  9255,  7659,  2797,  9898,  9346,  8251, 12037, 11138,  6447,
	11764,  2268, 10359,  3422,  9230,  1909, 11694,  7486,  8378,  8539,
	 8913,  3770,  3920,  2728,  6218,  8039, 11780,  3182,  1757,  6665,
	  639,  1172,  5158,  2787,  3605,  1631,  5060,   261,  2162,  9831,
	 8182,  3487, 11425, 12089,  9815,  9213,  9221,  2931,  8852,  7966,
	11962,  4362, 11438,  5151,  8909,  9686,  4545,    28, 11662,  5658,
	 6824,  8862,  7161,  1999,  4205, 11328,  3475,  9566, 10434,  3098,
	12055,  1994, 12131,   191
};

/* Constants are public so that they can be shared with alternate
   implementations. */
ALIGN32
const uint16_t mq_iGM[] = {
	 5476,   553,  5310,   819,  1446,   348,  3386,  6271,  9508,  3716,
	11437,  5659,  5850,   694,  4775,  8339, 12191,  2526,  2966, 11830,
	  405,  9123,  9311,  7289,  8986,  5885,  8175, 10738, 10766,  8659,
	  700,  3024,  6229,  8230,  8603,  4722,  5231,  6868,   436,  5816,
	 8679,  6525,  8187,  3908,  7395, 12284,  1152,  7926,  2586,  2815,
	 2741, 10858, 11383, 11816,   836,  7544, 10666,  8227, 11752,  4562,
	  312,  6755,  4351,  7982,  8158, 10173,   882,  1844,  4156,  2224,
	 8644,  3916, 10619,   159,  5149,  8480,  2638,  5989,  1418,  8092,
	 1775,  7668,   451,  3423,  1317,  6181,  8795,  6043,  7283,  6393,
	11564,  9157, 12161,  7312,  1366,  4918, 11699, 12198,  1304, 11532,
	 6451,  4765,  8154,  4257,  4191,  4833,  2318, 11980, 10393,  9997,
	 9481,   650, 10594,    51,  7912,  2720,  9889,  1921,  7179,    45,
	 3188,  8365,  2077, 11922,  5384, 11953,  8596,  6658, 10981,  7130,
	 3974,  3404, 12177,  6398, 10412,  1231,  8833,   800,    15,  9896,
	 5003,  1459,   565, 12272,  9781,  1946,  1419,  9571,  3844,  7758,
	 4293,  8223, 11525,   632,  4313,   936, 12186,  7420, 10892, 10678,
	 8934,  2711,  9498,  1215,  4711, 11995,  1377,  8898, 10189,  3217,
	10890,  7720,  6923,  2380,  4653, 12236, 10253, 11850, 10207,  5261,
	 1141,  3946,  7601,  9733, 10630,  4139,  9832,  3641, 11245,  4338,
	 5765, 10158,   116, 11807, 10283,  7064,   273, 10519,  2271,  3912,
	 8424, 10339,  6876,  6601, 10079,   284,   927,  6954,  3041, 12154,
	 6526,  5089, 12136,  7204,  4129, 11447, 11079,  4604,  1008,  3863,
	11772,  9564,  1101,  6231, 10482,  6449,  6035,  3951,  1574,  5325,
	 2020,  1353,  8191,  9824,  2642, 11905,  5399,  9560,  9396, 10114,
	 8035,   302,  6611,  7914, 11812,  7279, 11427,  3158,  6348, 12185,
	 6757,  2646,  5617,   179,   541,  1354,  9642,  5278, 10391,  7039,
	 6801,  6277,  6339, 11163,  2702,  2333,   735,  5633, 11656, 10046,
	 3107, 11456, 12287,  9331,  8086,  1997,  4021, 11472,  1444,  9679,
	11720,  6390,  2424,  8997,  7242,  7199,  5281,  7084,  7651,  9949,
	10709, 10379,  9637, 10172,  6028,  5887,  7701, 10165,  5183,  9610,
	 7424,  6019,  6795,  9692, 10837,  3067,  8583, 12009,  6753,  9019,
	 3779,  9935,  4732,  6187,  2497,  6363, 10289,  3649, 12127,  6182,
	 5684,   960,    18,  2044,  1088, 11582,   678,  7353,  7239,  2762,
	 5121,  3935,  2311,  1627,  8556,  8943,  1541,  5674,   260,  3581,
	 4792,  8904,  5697,  7898,  2155,  4394,   236,  4952, 11534,  1654,
	 4793, 10383,  9769,  8776,   779,  9264,  3392,  2856,   668,  4852,
	 8111,  2105,  6568,  5762,  6482,  1458,  5711,  4026,   467,  2509,
	 4425,  6827,  1205,   290,  6918,  7274,  3827,  7193, 11579,  6764,
	 4875,  8771,  1931,  4901,  6494,  6917,  6351,  4333,  7020, 10664,
	 5730,  7549,  4193,  7791,  6521,  9983,  6372, 10814,  8037,  3260,
	  952,  7062,  9810,  7970,  3088,  7933,   840,  1171,   486,  6032,
	 1342,  6289,  6017,  1907,  5489,  7491,  7957,  7830,  2451, 12063,
	 8874, 12283,  6298, 11969,  8735,  3326,  2981,  9437,  5408, 10582,
	 9876,  7272,  2968,  2499,  6729, 10390,  5290,  8106,  7679,  2205,
	 8744,  4348,  3461,  6595,  5747,  8114,  3378,  6728, 10285, 10022,
	 3721, 10176, 10539,  4729,  9075,  2337,  7445,   211,  7915,  7157,
	 5974, 12044,  7292,  7415,  3824,  2756, 11419,  3615,  4762,  1401,
	 4097,   986,  6496,  9875, 10554,  2336,  2999, 11481,  4286, 10159,
	 7487,   884, 10155,  2087,  7556,  4623,  1546,   780, 10199,  5718,
	 7327, 10024, 11396,  6465,  9722,   708, 11199, 10038,  7408,  6933,
	 4003,  9428,   484,  3074,  9409,  4763,  6157,    54,  2121,  3264,
	 2519,  2034,  6049,    79, 11292,   117, 10740,  7072,  7506,  4407,
	 6625,  4042,  5145,  2564,  7858,  8877,  9460,  6458, 12275,  3872,
	 7446,  1690,  3569,  6570, 10108,  6308,  8306,  7863,  4679,  1534,
	 1538,  1237,   100,   432,  4401,  8198,  1229, 11208,  6014,  9759,
	 5329,  4342,  4751,  9710, 11703,  5825,  2812,  5266, 10698,  6399,
	 2125,  9180, 10925, 10329, 10404,  1688,  1875,  8100,  8546,  6442,
	 5190,  7674, 10578,   965, 11155,  6407,  2921,  6720,   126,  2019,
	 7616,  7340,  4746,  2315,  1517,  7045, 11269,  2967,  3888, 11389,
	10736,  1156, 10787,  2851,  1820,   489,  8966,   883,  3012,  6130,
	 2796,  6180,  1652, 10086,  7004, 11578,  8973, 11236,  6938, 12276,
	 5453,  3403, 11455,  7703,  4676,  9386,  7621,  2446,  9109,  3467,
	 8507, 10206,  3110,  3604,  3269,  5274,  6397, 10922,  8435,  2030,
	11559,  1762,  2211,  1195,  7319, 10481,  9547, 12241,  1228,  9729,
	 8591, 11552,  7590,  5753, 12273,   914,  3243,  3687,  4773,  5381,
	 8780,  8436,  7737,  1964,  7103, 10531,  6664,   278,  7225,  6634,
	 9327,  6375,  5880,  8197,  3402,  5357,  9394,  7156,  5252,  1060,
	 1556,  3281,  6543,  5654,  4868, 10707,   673, 12247,  7219, 10049,
	11989, 10993,  8578,  4614,   989,   340,  7687,  1748,  8487,  5204,
	10236, 11285,   163,  7586,  4597,  3146, 12052,  5858, 11938,  9298,
	 3362,  7642, 11357, 10229, 10550,  8709,  1469,  9787,    39,  8525,
	 2080,  4070,  2959,  1477,  6249,   943,  4951, 10574,  1888,  2749,
	 2190,  7003,  6199,   727,  8756,  9807,  4101,  6902,  8605,  7680,
	  144,  4063,  8704,  6633,  5424,  9668,  3253,  6188,  9640,  2320,
	 3736,  7783, 10822,  5460,  9948,  3159,  2133,  8723,  6038,  8388,
	 6609,  4956,  4659,  8821,  2700, 11664,  3443,  4551,  3388,  9229,
	 4418,  8763,  6232,   378,  2558, 10559,  5344,  1949,  3133,   754,
	 3240, 11539, 11505,  7919, 11439,  8617,   386,  5600,   105,  7827,
	10443, 10213,  3955, 12170,  7022,  1333,  9933,  5552,  2330,  5150,
	 5473,  8405,  6941,  4424,  5613,  6552, 11568,  2784,  2510,  1012,
	 1093,  6688,  5041,  8505,  8399, 10231,  9639,   841,  9878, 10230,
	 2496,  4884, 11594,  4371,  7993, 11918, 10326,  9216, 10004, 12249,
	 7987,  3044,  4051,  6686,   676,  4395,  7379,   909,  4981,  5788,
	 3488,  9661,   812,  8915, 10536,   292,  1911, 12188,  3608,  2806,
	 9812, 10928, 11265,  9340,  9108,  1988,  6489, 11811,  8998, 11344,
	 8815, 11045, 11218,  1272,  4325,  6395,  3819,  7650,  7056,  2463,
	 8670,  5503,  7707,  6750, 11929,  8276,  5378,  3079, 11018,   408,
	 1851,  9471,  8181,  7323,  6205,  9601,   926,  5475,  4327,  9353,
	 7089,  2114,  9410,  6242,  8950,  1797,  6255,  9817,  7569, 11561,
	10432,  6233,  2452,  1253,  3787,  9478,  7950,  9766,  6174,   619,
	 4514,  3279, 11352,  2834,   599,  1113, 11465, 10204,  6177,  5056,
	 9926,  7488,   136,  4520,  3157, 11672,  9219,  6400,   120,  5434,
	 1825,  7884,  7214,  2654, 11393,  2028,  9562,  9848,  8159, 11652,
	 9128,  6990,  8290,  8777,  7922,  5221,  4759,  9253,  3937, 10126,
	11306,  8534,  4922,  4550,   424,   357,  6228,  6751,  7778,  1158,
	 1097,   315, 10027,  9399,  2250,  9720,   821,  9937, 11016,  9739,
	 6736,  8454, 11065,  8476, 12039, 11209,  3052,  3845, 11597,  8808,
	 8153,  2778,  2609, 12254,  8064,  6326,  5813,  7416,  6898,  2272,
	 5947,  8978,  5852,  3652,   928,  8433,  8530,  7356,  2184, 10418,
	 5879,  6718, 11603,  5393,  8473,  9076,  2835,  2416,  3732,  1867,
	 1457,  4328,  8069,  1432,  1628, 11457,  4900,  8879,  5111,  1434,
	 6325,  2746,  4083,  4858,  8847,  9217, 10122,  2436, 11413,  7030,
	  303,  5733,  3871, 10824
};
