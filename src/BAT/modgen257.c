
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
	x = 257 - (x + y);

	/* Add q if the value is strictly negative. Note that since
	   x <= q and y <= q, a negative value will have its
	   top 16 bits all equal to 1. */
	x += 257 & (x >> 16);

	/* Since we have -(x+y) in the 0..q-1 range, we can get
	   x+y = -(-(x+y)) in the 1..q range. */
	return 257 - x;
}

__attribute__ ((unused))
static inline uint32_t
mq_mul2(uint32_t x)
{
	/* Compute -2*x in the -q..q-2 range. */
	x = 257 - (x << 1);

	/* Add q if the value is strictly negative. Note that since
	   x <= q, a negative value will have its top 16 bits all equal to 1. */
	x += 257 & (x >> 16);

	/* Since we have -2*x in the 0..q-1 range, we can get
	   2*x = -(-2*x) in the 1..q range. */
	return 257 - x;
}



__attribute__ ((unused))
static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/* Get y-x in the -q+1..q-1 range. */
	y -= x;

	/* Add q if the value is strictly negative. New range is 0..q-1 */
	y += 257 & (y >> 16);

	/* Return -(y-x) = x-y. */
	return 257 - y;
}



__attribute__ ((unused))
static inline uint32_t
mq_neg(uint32_t x)
{
	x = 257 - x;
	x += 257 & ((x - 1) >> 16);
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

	x *= 16711935;
	x = (x >> 16) * 257;
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
	return mq_montyred((x + 257) * 1);
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
		+ (int32_t)257 * (1 + ((int32_t)503109 / 257))) * 1);
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
	x &= (uint32_t)(x - 257) >> 16;
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
	return (int)x - (int)(257 & ((uint32_t)((257 / 2) - x) >> 16));
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



	uint32_t y;

	/* x -> x^3 */
	y = mq_montymul(x, x);
	x = mq_montymul(y, x);

	/* x^3 -> x^15 */
	y = mq_montymul(x, x);
	y = mq_montymul(y, y);
	x = mq_montymul(y, x);

	/* x^15 -> x^255 = 1/x */
	y = mq_montymul(x, x);
	y = mq_montymul(y, y);
	y = mq_montymul(y, y);
	y = mq_montymul(y, y);
	x = mq_montymul(y, x);

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



/* q = 257, w = 3, 1/w = 86 */

ALIGNED_AVX2
static const uint16_t GM[] = {
	   1,  241,   64,    4,  249,  128,    2,  225,  136,  137,
	 223,   30,  197,  189,   15,   17,   81,  246,   44,   67,
	 123,   88,  162,  235,  222,   46,   73,  117,   23,  146,
	 187,   92,    9,  113,   62,   36,  185,  124,   18,  226,
	 196,  205,  208,   13,  231,  159,  135,  153,  215,  158,
	 139,   89,   79,   21,  173,   59,  199,  157,  143,   25,
	 207,   29,  141,   57,    3,  209,  192,   12,  233,  127,
	   6,  161,  151,  154,  155,   90,   77,   53,   45,   51,
	 243,  224,  132,  201,  112,    7,  229,  191,  152,  138,
	 219,   94,   69,  181,   47,   19,   27,   82,  186,  108,
	  41,  115,   54,  164,   74,  101,  110,   39,  179,  220,
	 148,  202,  131,  217,  160,   10,  237,   63,    5,  177,
	  83,  214,  172,   75,  107,   87,  166,  171
};

ALIGNED_AVX2
static const uint16_t iGM[] = {
	   1,   16,  253,  193,   32,  255,  129,    8,  240,  242,
	  68,   60,  227,   34,  120,  121,  165,   70,  111,  234,
	 140,  184,  211,   35,   22,   95,  169,  134,  190,  213,
	  11,  176,  200,  116,  228,   50,  232,  114,  100,   58,
	 198,   84,  236,  178,  168,  118,   99,   42,  104,  122,
	  98,   26,  244,   49,   52,   61,   31,  239,  133,   72,
	 221,  195,  144,  248,   86,   91,  170,  150,  182,   85,
	  43,  174,   80,  252,  194,   20,  247,   97,   40,  126,
	  55,  109,   37,   78,  218,  147,  156,  183,   93,  203,
	 142,  216,  149,   71,  175,  230,  238,  210,   76,  188,
	 163,   38,  119,  105,   66,   28,  250,  145,   56,  125,
	  33,   14,  206,  212,  204,  180,  167,  102,  103,  106,
	  96,  251,  130,   24,  245,   65,   48,  254
};

ALIGNED_AVX2
static const uint16_t NX[] = {
	   3,  254,  209,   48,  192,   65,   12,  245,  233,   24,
	 127,  130,    6,  251,  161,   96,  151,  106,  154,  103,
	 155,  102,   90,  167,   77,  180,   53,  204,   45,  212,
	  51,  206,  243,   14,  224,   33,  132,  125,  201,   56,
	 112,  145,    7,  250,  229,   28,  191,   66,  152,  105,
	 138,  119,  219,   38,   94,  163,   69,  188,  181,   76,
	  47,  210,   19,  238,   27,  230,   82,  175,  186,   71,
	 108,  149,   41,  216,  115,  142,   54,  203,  164,   93,
	  74,  183,  101,  156,  110,  147,   39,  218,  179,   78,
	 220,   37,  148,  109,  202,   55,  131,  126,  217,   40,
	 160,   97,   10,  247,  237,   20,   63,  194,    5,  252,
	 177,   80,   83,  174,  214,   43,  172,   85,   75,  182,
	 107,  150,   87,  170,  166,   91,  171,   86
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

				d[j + t] = mq_montyred((257 + u - v) * s);

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
		ni = mq_montyred(64 << (10 - logn));
	} else {
		ni = 255;
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
			z &= a[u] - 257;
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
			c = mq_montyred(((uint32_t)((uint32_t)257 * (uint32_t)257)) + a0 * a0 - NX[u >> 1] * c);

			z &= c - 257;
			c = mq_inv(c);

			d[u] = mq_montyred(a0 * c);
			d[u + 1] = mq_montyred(a1 * (2 * 257 - c));

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
				2 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) + a2 * a2 - 2 * a1 * a3));
			b1 = mq_montyred(2 * ((uint32_t)((uint32_t)257 * (uint32_t)257))
				+ 2 * a0 * a2 - a1 * a1
				- x * mq_montyred(a3 * a3));
			c = mq_inv(mq_montyred(
				((uint32_t)((uint32_t)257 * (uint32_t)257)) + b0 * b0 - x * mq_montyred(b1 * b1)));
			z &= c - 257;
			b0 = mq_montyred(b0 * c);
			b1 = mq_montyred(b1 * (2 * 257 - c));

			d[u] = mq_montyred(a0 * b0 + x * mq_montyred(a2 * b1));
			d[u + 1] = mq_montyred(3 * ((uint32_t)((uint32_t)257 * (uint32_t)257))
				- a1 * b0 - x * mq_montyred(a3 * b1));
			d[u + 2] = mq_montyred(a2 * b0 + a0 * b1);
			d[u + 3] = mq_montyred(3 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a3 * b0 - a1 * b1);

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
				4 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) + a4 * a4
				+ 2 * (a2 * a6 - a1 * a7 - a3 * a5)));
			b1 = mq_montyred(((uint32_t)((uint32_t)257 * (uint32_t)257)) + 2 * a0 * a2 - a1 * a1
				+ x * mq_montyred(3 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a5 * a5
				+ 2 * (a4 * a6 - a3 * a7)));
			b2 = mq_montyred(2 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) + a2 * a2
				+ 2 * (a0 * a4 - a1 * a3)
				+ x * mq_montyred(2 * ((uint32_t)((uint32_t)257 * (uint32_t)257))
				+ a6 * a6 - 2 * a5 * a7));
			b3 = mq_montyred(4 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a3 * a3
				+ 2 * (a0 * a6 + a2 * a4 - a1 * a5)
				- x * mq_montyred(a7 * a7));

			c0 = mq_montyred(b0 * b0 + x * mq_montyred(
				2 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) + b2 * b2 - 2 * b1 * b3));
			c1 = mq_montyred(2 * ((uint32_t)((uint32_t)257 * (uint32_t)257))
				+ 2 * b0 * b2 - b1 * b1
				- x * mq_montyred(b3 * b3));
			e = mq_inv(mq_montyred(
				((uint32_t)((uint32_t)257 * (uint32_t)257)) + c0 * c0 - x * mq_montyred(c1 * c1)));
			z &= e - 257;
			c0 = mq_montyred(c0 * e);
			c1 = mq_montyred(c1 * (2 * 257 - e));

			f0 = mq_montyred(b0 * c0 + x * mq_montyred(b2 * c1));
			f1 = mq_montyred(3 * ((uint32_t)((uint32_t)257 * (uint32_t)257))
				- b1 * c0 - x * mq_montyred(b3 * c1));
			f2 = mq_montyred(b2 * c0 + b0 * c1);
			f3 = mq_montyred(3 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - b3 * c0 - b1 * c1);

			d[u] = mq_montyred(a0 * f0 + x * mq_montyred(
				a2 * f3 + a4 * f2 + a6 * f1));
			d[u + 1] = mq_montyred(3 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a1 * f0
				- x * mq_montyred(a3 * f3 + a5 * f2 + a7 * f1));
			d[u + 2] = mq_montyred(a0 * f1 + a2 * f0
				+ x * mq_montyred(a4 * f3 + a6 * f2));
			d[u + 3] = mq_montyred(4 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a1 * f1 - a3 * f0
				- x * mq_montyred(a5 * f3 + a7 * f2));
			d[u + 4] = mq_montyred(a0 * f2 + a2 * f1
				+ a4 * f0 + x * mq_montyred(a6 * f3));
			d[u + 5] = mq_montyred(5 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a1 * f2 - a3 * f1
				- a5 * f0 - x * mq_montyred(a7 * f3));
			d[u + 6] = mq_montyred(a0 * f3 + a2 * f2
				+ a4 * f1 + a6 * f0);
			d[u + 7] = mq_montyred(5 * ((uint32_t)((uint32_t)257 * (uint32_t)257)) - a1 * f3 - a3 * f2
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
	 242,   17
};

static const uint16_t TT2[] = {
	  53,  174,   85,  206
};

static const uint16_t TT3[] = {
	 143,  220,   87,    4,  255,  172,   39,  116
};

static const uint16_t TT4[] = {
	  59,  227,   34,  149,  213,  218,  124,  141,  118,  135,
	  41,   46,  110,  225,   32,  200
};

static const uint16_t TT5[] = {
	 212,  163,   43,  154,  245,   80,  109,  189,  198,  228,
	 127,   52,  166,   82,  123,  159,  100,  136,  177,   93,
	 207,  132,   31,   61,   70,  150,  179,   14,  105,  216,
	  96,   47
};

static const uint16_t TT6[] = {
	  64,  103,   78,  248,  139,  204,   44,    7,   81,  152,
	 234,  183,   15,  203,  241,  137,  199,  197,  194,    5,
	  72,  182,  214,  147,  219,  113,   13,  151,   23,  223,
	  71,  247,   12,  188,   36,  236,  108,  246,  146,   40,
	 112,   45,   77,  187,  254,   65,   62,   60,  122,   18,
	  56,  244,   76,   25,  107,  178,  252,  215,   55,  120,
	  11,  181,  156,  195
};

static const uint16_t TT7[] = {
	 256,  129,   42,  164,  148,    8,  140,   99,  144,  134,
	 155,  253,   51,   37,  106,  165,  233,  186,  173,  131,
	  10,  201,  205,  161,  142,  145,  168,  238,   35,  190,
	 185,   89,  240,  158,  121,   16,  102,   29,  239,   28,
	 169,  232,  171,  193,  133,   38,  211,   83,   97,   84,
	  30,  196,   33,  250,  210,   92,   68,  235,  237,  209,
	  67,   75,   57,  180,   79,  202,  184,  192,   50,   22,
	  24,  191,  167,   49,    9,  226,   63,  229,  175,  162,
	 176,   48,  221,  126,   66,   88,   27,   90,  231,   20,
	 230,  157,  243,  138,  101,   19,  170,   74,   69,  224,
	  21,   91,  114,  117,   98,   54,   58,  249,  128,   86,
	  73,   26,   94,  153,  222,  208,    6,  104,  125,  115,
	 160,  119,  251,  111,   95,  217,  130,    3
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

