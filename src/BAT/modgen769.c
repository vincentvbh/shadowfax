
/*
 * This file is not meant to be compiled independently, but to be
 * included (with #include) by another C file. The caller is supposed to
 * have already included <stdint.h> and <string.h>; moreover, the Q
 * macro should be defined to one of the supported modulus values.
 */

/*
 * This module implements operations modulo a prime q. It works for
 * a few specific primes (257, 769 and 3329); it could be extended to
 * other primes such that q < 65536 and q = 1 mod 256, provided that
 * the relevant constants are defined.
 *
 * Internally, all computations use a representation in 1..q range (i.e.
 * value 0 is represented by q, not by 0). Montgomery multiplication
 * uses R = 2^32, i.e. representation of x is x*2^32 mod q, in the 1..q
 * range. With R > q^2, Montgomery reduction can be done efficiently
 * since it does not require an extra conditional subtraction.
 *
 * Macro Q must be defined beforehand, to the modulus q.
 *   Q1I = -1/q mod 2^32
 *   R2  = 2^64 mod q
 *   T54 = 2^54 mod q
 *   T25 = 2^25 mod q
 *   T25iGM1 = iGM[1]/128 mod q
 * (iGM[1] is the second element of the table used for inverse NTT, see
 * below; iGM[] is in Montgomery representation, hence T25iGM1 is the
 * Montgomery representation of the actual value.)
 */

__attribute__ ((unused))
static inline uint32_t
mq_add(uint32_t x, uint32_t y)
{
	/* Compute -(x+y) in the -q..q-2 range. */
	x = 769 - (x + y);

	/* Add q if the value is strictly negative. Note that since
	   x <= q and y <= q, a negative value will have its
	   top 16 bits all equal to 1. */
	x += 769 & (x >> 16);

	/* Since we have -(x+y) in the 0..q-1 range, we can get
	   x+y = -(-(x+y)) in the 1..q range. */
	return 769 - x;
}

__attribute__ ((unused))
static inline uint32_t
mq_mul2(uint32_t x)
{
	/* Compute -2*x in the -q..q-2 range. */
	x = 769 - (x << 1);

	/* Add q if the value is strictly negative. Note that since
	   x <= q, a negative value will have its top 16 bits all equal to 1. */
	x += 769 & (x >> 16);

	/* Since we have -2*x in the 0..q-1 range, we can get
	   2*x = -(-2*x) in the 1..q range. */
	return 769 - x;
}



__attribute__ ((unused))
static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/* Get y-x in the -q+1..q-1 range. */
	y -= x;

	/* Add q if the value is strictly negative. New range is 0..q-1 */
	y += 769 & (y >> 16);

	/* Return -(y-x) = x-y. */
	return 769 - y;
}



__attribute__ ((unused))
static inline uint32_t
mq_neg(uint32_t x)
{
	x = 769 - x;
	x += 769 & ((x - 1) >> 16);
	return x;
}



/*
 * Given input x, compute x/2^32 modulo q. Returned value is in the 1..q
 * range.
 *
 * IF q <= 40504:
 * ==============
 *
 * This function works for all 1 <= x <= 2^32 + 2^16 - 1 - (2^16-1)*q.
 * The upper limit is, for the supported values of q:
 *     q      limit
 *    257   4278190336
 *    769   4244636416
 *   3329   4076866816
 * If q <= 19433, then the limit is greater than 8*q^2, so that we may
 * then add together up to 8 simple products (over the integers) in order
 * to mutualize the Montgomery reduction.
 *
 * IF q > 40504:
 * =============
 *
 * When q is larger than 40504, the validity range of the method above is
 * lower than q^2, which means that it is not sufficient to implement
 * multiplications. In that case, the function below uses a different
 * technique which involves a 32x32->64 multiplication, but works for all
 * values in the 1..2^32-1 range.
 */
__attribute__ ((unused))
static inline uint32_t
mq_montyred(uint32_t x)
{

	x *= 452395775;
	x = (x >> 16) * 769;
	return (x >> 16) + 1;

}



/*
 * Given x and y, compute (x*y)/2^32 mod q; returned value is in 1..q.
 * Function works as long as 1 <= x*y <= 2^32 + 2^16 - 1 - (2^16-1)*q.
 */
__attribute__ ((unused))
static inline uint32_t
mq_montymul(uint32_t x, uint32_t y)
{
	return mq_montyred(x * y);
}





/*
 * Given x, compute x*2^32 mod q. This works for all x (including zero)
 * up to some limit, which depends on the modulus:
 *     q      limit
 *    257   4278190079
 *    769     11757226
 *   3329      1361084
 *  64513       954700
 */
__attribute__ ((unused))
static inline uint32_t
mq_tomonty(uint32_t x)
{
	return mq_montyred((x + 769) * 361);
}

/*
 * Given a signed integer x (in the -503109..+503109 range), obtain the
 * Montgomery representation of x (i.e. x*2^32 mod q, in the 1..q range).
 */
__attribute__ ((unused))
static inline uint32_t
mq_set(int32_t x)
{
	return mq_montyred((uint32_t)((int32_t)x
		+ (int32_t)769 * (1 + ((int32_t)503109 / 769))) * 361);
}

/*
 * Convert back an integer from Montgomery representation (in 1..q) into
 * an unsigned integer in normal representation, in the 0..q-1 range.
 */
__attribute__ ((unused))
static inline uint32_t
mq_unorm(uint32_t x)
{
	x = mq_montyred(x);
	x &= (uint32_t)(x - 769) >> 16;
	return x;
}

/*
 * Convert back an integer from Montgomery representation (in 1..q) into
 * a signed integer in normal representation, in the -((q-1)/2)..+((q-1)/2)
 * range.
 */
__attribute__ ((unused))
static inline int
mq_snorm(uint32_t x)
{
	x = mq_montyred(x);
	return (int)x - (int)(769 & ((uint32_t)((769 / 2) - x) >> 16));
}

/*
 * Invert x modulo q. Input and output are both in Montgomery representation.
 * If x = 0, then 0 is returned.
 */
__attribute__ ((unused))
static inline uint32_t
mq_inv(uint32_t x)
{
	/*
	 * We use Fermat's little theorem: 1/x = x^(q-2) mod q.
	 * An efficient addition chain on the exponent is used; the
	 * chain depends on the modulus.
	 */



	uint32_t x2, x3, x5, x10, x13;

	x2 = mq_montymul(x, x);
	x3 = mq_montymul(x2, x);
	x5 = mq_montymul(x3, x2);
	x10 = mq_montymul(x5, x5);
	x13 = mq_montymul(x10, x3);
	x = mq_montymul(x13, x10);   /* x^23 */
	x = mq_montymul(x, x);       /* x^46 */
	x = mq_montymul(x, x);       /* x^92 */
	x = mq_montymul(x, x);       /* x^184 */
	x = mq_montymul(x, x);       /* x^368 */
	x = mq_montymul(x, x13);     /* x^381 */
	x = mq_montymul(x, x);       /* x^762 */
	x = mq_montymul(x, x5);      /* x^767 = 1/x */

	return x;


}



/*
 * NTT.
 *
 * Let rev_d() be the bit reversal function over d bits: for an
 * integer a \in 0..2^d-1, we represent it in base 2 over d bits:
 *   a = \sum_{i=0}^{d-1} a_i * 2^i
 * Then, its bit-reverse is:
 *   rev_d(a) = \sum_{i=0}^{d-1} a_i 2^(d-1-i)
 *
 * The NTT representation of a polynomial f modulo X^n+1, for degree
 * n = 2^d and 1 <= d <= 7, is the set of values f(w^(2*i+1)), where
 * w is a primitive 2n-th root of 1 modulo q (i.e. w^n = -1 mod q).
 * The NTT representation is in bit-reversal order:
 *   f_NTT[rev_d(i)] = f(w^(2*i+1))  for i in 0..n-1
 *
 * The choice of the root w is purely conventional; amy will do, as long
 * as the NTT representation is not part of the public API, and the
 * NTT() and iNTT() functions use the same root.
 *
 * For larger degrees (d = 8, 9 or 10, for n = 256, 512 or 1024), a
 * partial NTT is used:
 *
 *  - For n = 256, polynomial f modulo X^256+1 is split into even and
 *    odd coefficients:
 *       f = f_0(X^2) + X*f_1(X^2)
 *    with both f_0 and f_1 being polynomials modulo X^128+1. Then,
 *    f_NTT is such that:
 *       f_NTT[0], f_NTT[2], f_NTT[4],... is the NTT representation of f_0
 *       f_NTT[1], f_NTT[3], f_NTT[5],... is the NTT representation of f_1
 *
 *  - For n = 512, polynomial f modulo X^512 is split into four
 *    sub-polynomials modulo X^128+1:
 *       f = f_0(X^4) + X*f_1(X^4) + X^2*f_2(X^4) + X^3*f_3(X^4)
 *    and the NTT representations of f_0, f_1, f_2 and f_3 are interleaved:
 *       f_NTT[0], f_NTT[4], f_NTT[8],... is the NTT representation of f_0
 *       f_NTT[1], f_NTT[5], f_NTT[9],... is the NTT representation of f_1
 *       f_NTT[2], f_NTT[6], f_NTT[10],... is the NTT representation of f_2
 *       f_NTT[3], f_NTT[7], f_NTT[11],... is the NTT representation of f_3
 *
 *  - For n = 1024, the split is into eight sub_polynomials:
 *       f = f_0(X^8) + X*f_1(X^8) + X^2*f_2(X^8) + ... + X^7*f_7(X^8)
 *    and the eight NTT representations are interleaved:
 *       f_NTT[0], f_NTT[8], f_NTT[16],... is the NTT representation of f_0
 *       f_NTT[1], f_NTT[9], f_NTT[17],... is the NTT representation of f_1
 *       f_NTT[2], f_NTT[10], f_NTT[18],... is the NTT representation of f_2
 *       f_NTT[3], f_NTT[11], f_NTT[19],... is the NTT representation of f_3
 *       f_NTT[4], f_NTT[12], f_NTT[20],... is the NTT representation of f_4
 *       f_NTT[5], f_NTT[13], f_NTT[21],... is the NTT representation of f_5
 *       f_NTT[6], f_NTT[14], f_NTT[22],... is the NTT representation of f_6
 *       f_NTT[7], f_NTT[15], f_NTT[23],... is the NTT representation of f_7
 *
 *
 * Addition and subtraction of polynomials is done coefficient-wise, both
 * in NTT and normal representation. For degrees up to 128, multiplication
 * is done coefficient-wise as well. For degrees 256, 512 and 1024,
 * multiplication is done on the NTT coefficients by groups of 2, 4 or 8,
 * respectively.
 *
 *
 * In the tables below:
 *   GM[rev(i)] = w^i  for i in 0..127, with w^128 = -1 mod q
 *   iGM[rev(i)] = w^(-i)
 *   NX is the NTT representation of polynomial X:
 *      NX[2 * i] = GM[64 + i]
 *      NX[2 * i + 1] = -GM[64 + i]
 * All values are in Montgomery representation.
 */



/* q = 769, w = 343, 1/w = 630 */

ALIGNED_AVX2
static const uint16_t GM[] = {
	  19,  360,  211,  760,  455,  243,  277,  513,  155,  387,
	 669,   48,  393,  242,  317,  340,  447,  739,  431,  193,
	 667,  172,   41,  534,  692,  160,  521,  765,  544,  108,
	 294,  228,  617,  196,  619,   72,  205,  363,   91,  510,
	 298,  749,   31,  385,  701,  371,  540,  356,  269,  240,
	 397,  763,   47,  162,  441,  342,  616,  258,  446,   32,
	 262,  674,  724,  483,  365,  440,   87,  758,  727,  297,
	 424,  627,  104,  473,  305,  315,  224,  723,  302,  501,
	 290,  476,  185,   65,  388,  552,  221,  140,  504,  281,
	 295,  166,  494,  132,  103,  535,  156,  325,   73,   88,
	 336,  700,  453,  367,  706,   61,  636,  556,  515,  368,
	 660,  606,  756,   37,   58,  249,  741,  198,  539,  418,
	 582,   59,  716,  210,  662,  482,  714,  334
};

ALIGNED_AVX2
static const uint16_t iGM[] = {
	  19,  409,    9,  558,  256,  492,  526,  314,  429,  452,
	 527,  376,  721,  100,  382,  614,  541,  475,  661,  225,
	   4,  248,  609,   77,  235,  728,  597,  102,  576,  338,
	  30,  322,  286,   45,   95,  507,  737,  323,  511,  153,
	 427,  328,  607,  722,    6,  372,  529,  500,  413,  229,
	 398,   68,  384,  738,   20,  471,  259,  678,  406,  564,
	 697,  150,  573,  152,  435,   55,  287,  107,  559,   53,
	 710,  187,  351,  230,  571,   28,  520,  711,  732,   13,
	 163,  109,  401,  254,  213,  133,  708,   63,  402,  316,
	  69,  433,  681,  696,  444,  613,  234,  666,  637,  275,
	 603,  474,  488,  265,  629,  548,  217,  381,  704,  584,
	 293,  479,  268,  467,   46,  545,  454,  464,  296,  665,
	 142,  345,  472,   42,   11,  682,  329,  404
};

ALIGNED_AVX2
static const uint16_t NX[] = {
	 365,  404,  440,  329,   87,  682,  758,   11,  727,   42,
	 297,  472,  424,  345,  627,  142,  104,  665,  473,  296,
	 305,  464,  315,  454,  224,  545,  723,   46,  302,  467,
	 501,  268,  290,  479,  476,  293,  185,  584,   65,  704,
	 388,  381,  552,  217,  221,  548,  140,  629,  504,  265,
	 281,  488,  295,  474,  166,  603,  494,  275,  132,  637,
	 103,  666,  535,  234,  156,  613,  325,  444,   73,  696,
	  88,  681,  336,  433,  700,   69,  453,  316,  367,  402,
	 706,   63,   61,  708,  636,  133,  556,  213,  515,  254,
	 368,  401,  660,  109,  606,  163,  756,   13,   37,  732,
	  58,  711,  249,  520,  741,   28,  198,  571,  539,  230,
	 418,  351,  582,  187,   59,  710,  716,   53,  210,  559,
	 662,  107,  482,  287,  714,   55,  334,  435
};







/*
 * Convert an array to (partial) NTT representation. This function
 * accepts all degrees from 2 (logn = 1) to 1024 (logn = 10). Source (a)
 * and destination (d) may overlap.
 */
__attribute__ ((unused)) TARGET_AVX2
static void
NTT(uint16_t *d, const uint16_t *a, unsigned logn)
{


	unsigned n, t, m, mm;

	n = 1u << logn;
	if (d != a) {
		memmove(d, a, n * sizeof *a);
	}
	mm = (logn <= 7) ? n : 128;
	t = n;
	for (m = 1; m < mm; m <<= 1) {
		unsigned ht, i, j1;

		ht = t >> 1;
		for (i = 0, j1 = 0; i < m; i ++, j1 += t) {
			unsigned j, j2;
			uint32_t s;

			s = GM[m + i];
			j2 = j1 + ht;
			for (j = j1; j < j2; j ++) {
				uint32_t u, v;

				u = d[j];
				v = mq_montymul(d[j + ht], s);
				d[j] = mq_add(u, v);
				d[j + ht] = mq_sub(u, v);
			}
		}
		t = ht;
	}


}

/*
 * Apply the inverse (partial) NTT on an array; this reverts the effect
 * of the NTT() function. This function accepts all degrees from 2
 * (logn = 1) to 1024 (logn = 10). Source (a) and destination (d) may
 * overlap.
 */
__attribute__ ((unused)) TARGET_AVX2
static void
iNTT(uint16_t *d, const uint16_t *a, unsigned logn)
{


	unsigned n, t, m;
	uint32_t ni;

	n = 1u << logn;
	if (d != a) {
		memmove(d, a, n * sizeof *a);
	}
	if (logn <= 7) {
		t = 1;
		m = n;
	} else {
		t = 1u << (logn - 7);
		m = 128;
	}
	while (m > 1) {
		unsigned hm, dt, i, j1;

		hm = m >> 1;
		dt = t << 1;
		for (i = 0, j1 = 0; i < hm; i ++, j1 += dt) {
			unsigned j, j2;
			uint32_t s;

			j2 = j1 + t;
			s = iGM[hm + i];
			for (j = j1; j < j2; j ++) {
				uint32_t u, v;

				u = d[j];
				v = d[j + t];
				d[j] = mq_add(u, v);

				d[j + t] = mq_montyred((769 + u - v) * s);

			}
		}
		t = dt;
		m = hm;
	}

	/*
	 * We need to divide by n, which we do by a multiplication by
	 * 1/n. Since we use Montgomery representation with R = 2^32,
	 * we use 2^32/n = 2^(32 - logn). We start with 2^54 mod q,
	 * then do a left shift by 10-logn (thus yielding a value equal
	 * to 2^(64-logn) modulo q), and apply a Montgomery reduction.
	 *
	 * However, for logn > 7, we have a partial NTT, and thus must
	 * stop at n = 128; i.e. with want ni = 2^25 mod q' (Montgomery
	 * representation of 1/128).
	 */
	if (logn <= 7) {
		ni = mq_montyred(306 << (10 - logn));
	} else {
		ni = 655;
	}
	for (m = 0; m < n; m ++) {
		d[m] = mq_montymul(d[m], ni);
	}


}

/*
 * Polynomial addition (works both in NTT and normal representations).
 */
__attribute__ ((unused)) TARGET_AVX2
static void
mq_poly_add(uint16_t *d, const uint16_t *a, const uint16_t *b, unsigned logn)
{

	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		d[u] = mq_add(a[u], b[u]);
	}

}

/*
 * Polynomial addition (works both in NTT and normal representations).
 */
__attribute__ ((unused)) TARGET_AVX2
static void
mq_poly_sub(uint16_t *d, const uint16_t *a, const uint16_t *b, unsigned logn)
{

	size_t u, n;

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		d[u] = mq_sub(a[u], b[u]);
	}

}

/*
 * Multiplication of a polynomial by a constant c (modulo q). The constant
 * is provided as a normal signed integer.
 */
__attribute__ ((unused)) TARGET_AVX2
static void
mq_poly_mulconst(uint16_t *d, const uint16_t *a, int c, unsigned logn)
{

	size_t u, n;
	uint32_t cc;

	n = (size_t)1 << logn;
	cc = mq_set(c);
	for (u = 0; u < n; u ++) {
		d[u] = mq_montymul(a[u], cc);
	}

}

/*
 * Polynomial multiplication (NTT only).
 */
__attribute__ ((unused)) TARGET_AVX2
static void
mq_poly_mul_ntt(uint16_t *d, const uint16_t *a, const uint16_t *b,
	unsigned logn)
{


	size_t u;

	if (logn <= 7) {
		size_t n;

		n = (size_t)1 << logn;
		for (u = 0; u < n; u ++) {
			d[u] = mq_montymul(a[u], b[u]);
		}
		return;
	}

	switch (logn) {
	case 8:
		for (u = 0; u < 256; u += 2) {
			uint32_t a0, a1, b0, b1;

			a0 = a[u];
			a1 = a[u + 1];
			b0 = b[u];
			b1 = b[u + 1];

			d[u] = mq_montyred(
				a0 * b0 + mq_montymul(a1, b1) * NX[u >> 1]);
			d[u + 1] = mq_montyred(
				a1 * b0 + a0 * b1);

		}
		break;
	case 9:
		for (u = 0; u < 512; u += 4) {
			uint32_t a0, a1, a2, a3, b0, b1, b2, b3, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			b0 = b[u];
			b1 = b[u + 1];
			b2 = b[u + 2];
			b3 = b[u + 3];
			x = NX[u >> 2];

			d[u] = mq_montyred(a0 * b0
				+ x * mq_montyred(
					a1 * b3 + a2 * b2 + a3 * b1));
			d[u + 1] = mq_montyred(
				a0 * b1 + a1 * b0
				+ x * mq_montyred(a2 * b3 + a3 * b2));
			d[u + 2] = mq_montyred(
				a0 * b2 + a1 * b1 + a2 * b0
				+ x * mq_montyred(a3 * b3));
			d[u + 3] = mq_montyred(
				a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0);

		}
		break;
	case 10:
		for (u = 0; u < 1024; u += 8) {
			uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
			uint32_t b0, b1, b2, b3, b4, b5, b6, b7;
			uint32_t x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			a4 = a[u + 4];
			a5 = a[u + 5];
			a6 = a[u + 6];
			a7 = a[u + 7];
			b0 = b[u];
			b1 = b[u + 1];
			b2 = b[u + 2];
			b3 = b[u + 3];
			b4 = b[u + 4];
			b5 = b[u + 5];
			b6 = b[u + 6];
			b7 = b[u + 7];
			x = NX[u >> 3];

			d[u] = mq_montyred(
				a0 * b0
				+ x * mq_montyred(
					a1 * b7 + a2 * b6 + a3 * b5 + a4 * b4
					+ a5 * b3 + a6 * b2 + a7 * b1));
			d[u + 1] = mq_montyred(
				a0 * b1 + a1 * b0
				+ x * mq_montyred(
					a2 * b7 + a3 * b6 + a4 * b5
					+ a5 * b4 + a6 * b3 + a7 * b2));
			d[u + 2] = mq_montyred(
				a0 * b2 + a1 * b1 + a2 * b0
				+ x * mq_montyred(
					a3 * b7 + a4 * b6 + a5 * b5
					+ a6 * b4 + a7 * b3));
			d[u + 3] = mq_montyred(
				a0 * b3 + a1 * b2 + a2 * b1 + a3 * b0
				+ x * mq_montyred(
					a4 * b7 + a5 * b6 + a6 * b5 + a7 * b4));
			d[u + 4] = mq_montyred(
				a0 * b4 + a1 * b3 + a2 * b2 + a3 * b1 + a4 * b0
				+ x * mq_montyred(
					a5 * b7 + a6 * b6 + a7 * b5));
			d[u + 5] = mq_montyred(
				a0 * b5 + a1 * b4 + a2 * b3 + a3 * b2
				+ a4 * b1 + a5 * b0
				+ x * mq_montyred(a6 * b7 + a7 * b6));
			d[u + 6] = mq_montyred(
				a0 * b6 + a1 * b5 + a2 * b4 + a3 * b3
				+ a4 * b2 + a5 * b1 + a6 * b0
				+ x * mq_montyred(a7 * b7));
			d[u + 7] = mq_montyred(
				a0 * b7 + a1 * b6 + a2 * b5 + a3 * b4
				+ a4 * b3 + a5 * b2 + a6 * b1 + a7 * b0);

		}
		break;
	}


}

/*
 * Polynomial inversion (NTT only). Returned value is 1 on success, 0
 * on failure; a failure is reported if the polynomial is not invertible.
 * On failure, the contents are unpredictable.
 */
__attribute__ ((unused)) TARGET_AVX2
static int
mq_poly_inv_ntt(uint16_t *d, const uint16_t *a, unsigned logn)
{


	size_t u, n;
	uint32_t z;

	z = (uint32_t)-1;
	if (logn <= 7) {
		n = (size_t)1 << logn;
		for (u = 0; u < n; u ++) {
			z &= a[u] - 769;
			d[u] = mq_inv(a[u]);
		}
		return (int)(z >> 31);
	}

	/*
	 * For larger degrees, we split polynomial a[] into its odd and
	 * even coefficients:
	 *    a = a0(X^2) + X*a1(X^2)
	 * With a0 and a1 being half-degree polynomials (they operate
	 * modulo X^(n/2)+1).
	 *
	 * We then define an adjoint:
	 *    a' = a0(X^2) - X*a1(X^2)
	 * This yields:
	 *    a*a' = (a0^2)(X^2) - X^2*(a1^2)(X^2)
	 *         = (a0^2 - X*a1^2)(X^2)
	 * i.e. a*a' is a half-degree polynomial (composed with X^2).
	 *
	 * If we can invert a*a', then:
	 *    1/a = (1/(a*a')) * a'
	 * It can be shown that a*a' is invertible if and only if a is
	 * invertible.
	 *
	 * Thus, to invert a polynomial modulo X^n+1, we just have to
	 * invert another polynomial modulo X^(n/2)+1. We can apply this
	 * process recursively to get down to degree 128 (logn = 7),
	 * that we can handle with coefficient-wise inversion in NTT
	 * representation.
	 */
	switch (logn) {

	case 8:
		for (u = 0; u < 256; u += 2) {
			uint32_t a0, a1, c;

			a0 = a[u];
			a1 = a[u + 1];

			c = mq_montyred(a1 * a1);
			c = mq_montyred(((uint32_t)((uint32_t)769 * (uint32_t)769)) + a0 * a0 - NX[u >> 1] * c);

			z &= c - 769;
			c = mq_inv(c);

			d[u] = mq_montyred(a0 * c);
			d[u + 1] = mq_montyred(a1 * (2 * 769 - c));

		}
		return (int)(z >> 31);

	case 9:
		for (u = 0; u < 512; u += 4) {
			uint32_t a0, a1, a2, a3, b0, b1, c, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			x = NX[u >> 2];


			b0 = mq_montyred(a0 * a0 + x * mq_montyred(
				2 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) + a2 * a2 - 2 * a1 * a3));
			b1 = mq_montyred(2 * ((uint32_t)((uint32_t)769 * (uint32_t)769))
				+ 2 * a0 * a2 - a1 * a1
				- x * mq_montyred(a3 * a3));
			c = mq_inv(mq_montyred(
				((uint32_t)((uint32_t)769 * (uint32_t)769)) + b0 * b0 - x * mq_montyred(b1 * b1)));
			z &= c - 769;
			b0 = mq_montyred(b0 * c);
			b1 = mq_montyred(b1 * (2 * 769 - c));

			d[u] = mq_montyred(a0 * b0 + x * mq_montyred(a2 * b1));
			d[u + 1] = mq_montyred(3 * ((uint32_t)((uint32_t)769 * (uint32_t)769))
				- a1 * b0 - x * mq_montyred(a3 * b1));
			d[u + 2] = mq_montyred(a2 * b0 + a0 * b1);
			d[u + 3] = mq_montyred(3 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a3 * b0 - a1 * b1);

		}
		return (int)(z >> 31);

	case 10:
		for (u = 0; u < 1024; u += 8) {
			uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
			uint32_t b0, b1, b2, b3, c0, c1, e, x;
			uint32_t f0, f1, f2, f3;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			a4 = a[u + 4];
			a5 = a[u + 5];
			a6 = a[u + 6];
			a7 = a[u + 7];
			x = NX[u >> 3];


			b0 = mq_montyred(a0 * a0 + x * mq_montyred(
				4 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) + a4 * a4
				+ 2 * (a2 * a6 - a1 * a7 - a3 * a5)));
			b1 = mq_montyred(((uint32_t)((uint32_t)769 * (uint32_t)769)) + 2 * a0 * a2 - a1 * a1
				+ x * mq_montyred(3 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a5 * a5
				+ 2 * (a4 * a6 - a3 * a7)));
			b2 = mq_montyred(2 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) + a2 * a2
				+ 2 * (a0 * a4 - a1 * a3)
				+ x * mq_montyred(2 * ((uint32_t)((uint32_t)769 * (uint32_t)769))
				+ a6 * a6 - 2 * a5 * a7));
			b3 = mq_montyred(4 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a3 * a3
				+ 2 * (a0 * a6 + a2 * a4 - a1 * a5)
				- x * mq_montyred(a7 * a7));

			c0 = mq_montyred(b0 * b0 + x * mq_montyred(
				2 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) + b2 * b2 - 2 * b1 * b3));
			c1 = mq_montyred(2 * ((uint32_t)((uint32_t)769 * (uint32_t)769))
				+ 2 * b0 * b2 - b1 * b1
				- x * mq_montyred(b3 * b3));
			e = mq_inv(mq_montyred(
				((uint32_t)((uint32_t)769 * (uint32_t)769)) + c0 * c0 - x * mq_montyred(c1 * c1)));
			z &= e - 769;
			c0 = mq_montyred(c0 * e);
			c1 = mq_montyred(c1 * (2 * 769 - e));

			f0 = mq_montyred(b0 * c0 + x * mq_montyred(b2 * c1));
			f1 = mq_montyred(3 * ((uint32_t)((uint32_t)769 * (uint32_t)769))
				- b1 * c0 - x * mq_montyred(b3 * c1));
			f2 = mq_montyred(b2 * c0 + b0 * c1);
			f3 = mq_montyred(3 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - b3 * c0 - b1 * c1);

			d[u] = mq_montyred(a0 * f0 + x * mq_montyred(
				a2 * f3 + a4 * f2 + a6 * f1));
			d[u + 1] = mq_montyred(3 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a1 * f0
				- x * mq_montyred(a3 * f3 + a5 * f2 + a7 * f1));
			d[u + 2] = mq_montyred(a0 * f1 + a2 * f0
				+ x * mq_montyred(a4 * f3 + a6 * f2));
			d[u + 3] = mq_montyred(4 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a1 * f1 - a3 * f0
				- x * mq_montyred(a5 * f3 + a7 * f2));
			d[u + 4] = mq_montyred(a0 * f2 + a2 * f1
				+ a4 * f0 + x * mq_montyred(a6 * f3));
			d[u + 5] = mq_montyred(5 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a1 * f2 - a3 * f1
				- a5 * f0 - x * mq_montyred(a7 * f3));
			d[u + 6] = mq_montyred(a0 * f3 + a2 * f2
				+ a4 * f1 + a6 * f0);
			d[u + 7] = mq_montyred(5 * ((uint32_t)((uint32_t)769 * (uint32_t)769)) - a1 * f3 - a3 * f2
				- a5 * f1 - a7 * f0);

		}
		return (int)(z >> 31);

	default:
		/* normally unreachable if logn is correct */
		return 0;
	}


}

/*
 * TTx[] contains the NTT representation of 1+X+X^2+X^3+...+X^(n-1) for
 * degree n = 2^x (for 1 <= x <= 7).
 */



static const uint16_t TT1[] = {
	 379,  428
};

static const uint16_t TT2[] = {
	 581,  177,  630,  226
};

static const uint16_t TT3[] = {
	 531,  631,  498,  625,  182,  309,  176,  276
};

static const uint16_t TT4[] = {
	   6,  287,  370,  123,  103,  124,  585,  665,  142,  222,
	 683,  704,  684,  437,  520,   32
};

static const uint16_t TT5[] = {
	 390,  391,  360,  214,  547,  193,  482,  533,  400,  575,
	 518,  499,  107,  294,  130,  431,  376,  677,  513,  700,
	 308,  289,  232,  407,  274,  325,  614,  260,  593,  447,
	 416,  417
};

static const uint16_t TT6[] = {
	 346,  434,  539,  243,  432,  288,  175,  253,   54,  271,
	 521,  634,  524,  440,  755,  311,   36,  764,  334,   47,
	  68,  199,  330,  668,  574,  409,  247,  341,  344,  685,
	 169,  693,  114,  638,  122,  463,  466,  560,  398,  233,
	 139,  477,  608,  739,  760,  473,   43,    2,  496,   52,
	 367,  283,  173,  286,  536,  753,  554,  632,  519,  375,
	 564,  268,  373,  461
};

static const uint16_t TT7[] = {
	 598,   94,  528,  340,   12,  297,  588,  667,  327,  537,
	  14,  562,  640,  479,  143,  363,  471,  406,   56,  486,
	 304,  738,  460,   39,   64,  215,  508,  372,  492,  249,
	 174,  448,  545,  296,  234,  525,  672,  765,  653,  210,
	 121,   15,  557,  610,  202,  458,  369,  198,  582,  566,
	 144,  674,  237,  257,  649,   33,   60,  628,  749,  621,
	 559,  548,   91,  526,  281,  716,  259,  248,  186,   58,
	 179,  747,    5,  158,  550,  570,  133,  663,  241,  225,
	 609,  438,  349,  605,  197,  250,   23,  686,  597,  154,
	  42,  135,  282,  573,  511,  262,  359,  633,  558,  315,
	 435,  299,  592,  743,  768,  347,   69,  503,  321,  751,
	 401,  336,  444,  664,  328,  167,  245,   24,  270,  480,
	 140,  219,  510,   26,  467,  279,  713,  209
};



/*
 * Multiply a polynomial 'a' by 'ones', with 'ones' being the polynomial
 * 1+X+X^2+X^3+...+X^n. Source and destination are in NTT representation.
 */
__attribute__ ((unused)) TARGET_AVX2
static void
mq_poly_mul_ones_ntt(uint16_t *d, const uint16_t *a, unsigned logn)
{
	size_t u;

	switch (logn) {
	case 1:
		mq_poly_mul_ntt(d, a, TT1, logn);
		break;
	case 2:
		mq_poly_mul_ntt(d, a, TT2, logn);
		break;
	case 3:
		mq_poly_mul_ntt(d, a, TT3, logn);
		break;
	case 4:
		mq_poly_mul_ntt(d, a, TT4, logn);
		break;
	case 5:
		mq_poly_mul_ntt(d, a, TT5, logn);
		break;
	case 6:
		mq_poly_mul_ntt(d, a, TT6, logn);
		break;
	case 7:
		mq_poly_mul_ntt(d, a, TT7, logn);
		break;

	case 8:
		for (u = 0; u < 256; u += 2) {
			uint32_t a0, a1, b;

			a0 = a[u];
			a1 = a[u + 1];
			b = TT7[u >> 1];

			d[u] = mq_montyred(
				b * (a0 + mq_montyred(a1 * NX[u >> 1])));
			d[u + 1] = mq_montyred(b * (a0 + a1));

		}
		break;
	case 9:
		for (u = 0; u < 512; u += 4) {
			uint32_t a0, a1, a2, a3, b, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			b = TT7[u >> 2];
			x = NX[u >> 2];

			d[u] = mq_montyred(
				b * (a0 + mq_montyred(x * (a1 + a2 + a3))));
			d[u + 1] = mq_montyred(
				b * (a0 + a1 + mq_montyred(x * (a2 + a3))));
			d[u + 2] = mq_montyred(
				b * (a0 + a1 + a2 + mq_montyred(x * a3)));
			d[u + 3] = mq_montyred(
				b * (a0 + a1 + a2 + a3));

		}
		break;
	case 10:
		for (u = 0; u < 1024; u += 8) {
			uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
			uint32_t b, x;

			a0 = a[u];
			a1 = a[u + 1];
			a2 = a[u + 2];
			a3 = a[u + 3];
			a4 = a[u + 4];
			a5 = a[u + 5];
			a6 = a[u + 6];
			a7 = a[u + 7];
			b = TT7[u >> 3];
			x = NX[u >> 3];

			d[u] = mq_montyred(
				b * (a0 + mq_montyred(
				x * (a1 + a2 + a3 + a4 + a5 + a6 + a7))));
			d[u + 1] = mq_montyred(
				b * (a0 + a1 + mq_montyred(
				x * (a2 + a3 + a4 + a5 + a6 + a7))));
			d[u + 2] = mq_montyred(
				b * (a0 + a1 + a2 + mq_montyred(
				x * (a3 + a4 + a5 + a6 + a7))));
			d[u + 3] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + mq_montyred(
				x * (a4 + a5 + a6 + a7))));
			d[u + 4] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + mq_montyred(
				x * (a5 + a6 + a7))));
			d[u + 5] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + a5 + mq_montyred(
				x * (a6 + a7))));
			d[u + 6] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + a5 + a6
				+ mq_montyred(x * a7)));
			d[u + 7] = mq_montyred(
				b * (a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7));

		}
		break;
	}

}

/*
 * Add a constant value (in Montgomery representation) to a polynomial,
 * in NTT representation.
 */
__attribute__ ((unused)) TARGET_AVX2
static void
mq_poly_addconst_ntt(uint16_t *d, const uint16_t *a, uint32_t c, unsigned logn)
{


	size_t u, n;

	switch (logn) {
	case 8:
		memmove(d, a, 256 * sizeof *a);
		for (u = 0; u < 256; u += 2) {
			d[u] = mq_add(d[u], c);
		}
		break;

	case 9:
		memmove(d, a, 512 * sizeof *a);
		for (u = 0; u < 512; u += 4) {
			d[u] = mq_add(d[u], c);
		}
		break;

	case 10:
		memmove(d, a, 1024 * sizeof *a);
		for (u = 0; u < 1024; u += 8) {
			d[u] = mq_add(d[u], c);
		}
		break;
	default:
		n = (size_t)1 << logn;
		for (u = 0; u < n; u ++) {
			d[u] = mq_add(a[u], c);
		}
		break;
	}


}

