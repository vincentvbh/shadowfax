
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
	x = 64513 - (x + y);

	/* Add q if the value is strictly negative. Note that since
	   x <= q and y <= q, a negative value will have its
	   top 16 bits all equal to 1. */
	x += 64513 & (x >> 16);

	/* Since we have -(x+y) in the 0..q-1 range, we can get
	   x+y = -(-(x+y)) in the 1..q range. */
	return 64513 - x;
}

__attribute__ ((unused))
static inline uint32_t
mq_mul2(uint32_t x)
{
	/* Compute -2*x in the -q..q-2 range. */
	x = 64513 - (x << 1);

	/* Add q if the value is strictly negative. Note that since
	   x <= q, a negative value will have its top 16 bits all equal to 1. */
	x += 64513 & (x >> 16);

	/* Since we have -2*x in the 0..q-1 range, we can get
	   2*x = -(-2*x) in the 1..q range. */
	return 64513 - x;
}



__attribute__ ((unused))
static inline uint32_t
mq_sub(uint32_t x, uint32_t y)
{
	/* Get y-x in the -q+1..q-1 range. */
	y -= x;

	/* Add q if the value is strictly negative. New range is 0..q-1 */
	y += 64513 & (y >> 16);

	/* Return -(y-x) = x-y. */
	return 64513 - y;
}



__attribute__ ((unused))
static inline uint32_t
mq_neg(uint32_t x)
{
	x = 64513 - x;
	x += 64513 & ((x - 1) >> 16);
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

	x *= 3354459135;
	return (uint32_t)(((uint64_t)x * 64513) >> 32) + 1;

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
	return mq_montyred((x + 64513) * 4214);
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
		+ (int32_t)64513 * (1 + ((int32_t)503109 / 64513))) * 4214);
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
	x &= (uint32_t)(x - 64513) >> 16;
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
	return (int)x - (int)(64513 & ((uint32_t)((64513 / 2) - x) >> 16));
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



	uint32_t y, x3, x31;

	y = mq_montymul(x, x);      /* x^2 */
	x3 = mq_montymul(y, x);     /* x^3 */
	y = mq_montymul(x3, x3);    /* x^6 */
	y = mq_montymul(y, y);      /* x^12 */
	y = mq_montymul(y, x3);     /* x^15 */
	y = mq_montymul(y, y);      /* x^30 */
	x31 = mq_montymul(y, x);    /* x^31 */
	y = mq_montymul(x31, x31);  /* x^62 */
	y = mq_montymul(y, y);      /* x^124 */
	y = mq_montymul(y, y);      /* x^248 */
	y = mq_montymul(y, y);      /* x^496 */
	y = mq_montymul(y, y);      /* x^992 */
	y = mq_montymul(y, y);      /* x^1984 */
	y = mq_montymul(y, x31);    /* x^2015 */
	y = mq_montymul(y, y);      /* x^4030 */
	y = mq_montymul(y, y);      /* x^8060 */
	y = mq_montymul(y, y);      /* x^16120 */
	y = mq_montymul(y, y);      /* x^32240 */
	y = mq_montymul(y, y);      /* x^64480 */
	y = mq_montymul(y, x31);    /* x^64511 */

	return y;


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



/* q = 64513, w = 45056, 1/w = 10262 */

ALIGNED_AVX2
static const uint16_t GM[] = {
	14321, 37549, 22229, 48008, 45449, 33295, 30746, 44270,
	50769, 32369, 21408, 46914, 55118, 33528,  8851, 41654,
	44478, 35380, 27527, 36366, 20143, 11361, 25252, 30820,
	41567, 48474, 58272, 44960, 29104, 42082, 22935, 10681,
	16608, 19616, 30608, 23970, 62122, 49383, 19646, 21464,
	 7941, 26533, 36823, 19129, 10615,  9430, 56916, 53054,
	54464, 55130, 22562, 57724, 12296, 48209, 16446, 46274,
	56620,  8577, 29643, 46572, 32076, 11782, 63217, 19725,
	52463, 18832, 50012, 56584, 43011, 18731,  4127, 16186,
	10623, 36786, 24985, 53252, 33186,  1160, 35803, 14941,
	33449, 29563, 58600,  5322, 58637, 35074,  2844, 48108,
	30362, 21442, 17671,  9560, 18586,  9522, 54639, 40669,
	 3761, 54909, 44160, 44700,  7814, 11591, 51816, 32114,
	  598, 44958, 16267, 47057, 34571, 59975, 15546,   835,
	49003, 57754, 22131, 35462, 35445, 16507, 59171, 54723,
	33161, 12442, 46882, 62707, 60543, 36828, 56202, 63025
};

ALIGNED_AVX2
static const uint16_t iGM[] = {
	14321, 26964, 16505, 42284, 20243, 33767, 31218, 19064,
	22859, 55662, 30985,  9395, 17599, 43105, 32144, 13744,
	53832, 41578, 22431, 35409, 19553,  6241, 16039, 22946,
	33693, 39261, 53152, 44370, 28147, 36986, 29133, 20035,
	44788,  1296, 52731, 32437, 17941, 34870, 55936,  7893,
	18239, 48067, 16304, 52217,  6789, 41951,  9383, 10049,
	11459,  7597, 55083, 53898, 45384, 27690, 37980, 56572,
	43049, 44867, 15130,  2391, 40543, 33905, 44897, 47905,
	 1488,  8311, 27685,  3970,  1806, 17631, 52071, 31352,
	 9790,  5342, 48006, 29068, 29051, 42382,  6759, 15510,
	63678, 48967,  4538, 29942, 17456, 48246, 19555, 63915,
	32399, 12697, 52922, 56699, 19813, 20353,  9604, 60752,
	23844,  9874, 54991, 45927, 54953, 46842, 43071, 34151,
	16405, 61669, 29439,  5876, 59191,  5913, 34950, 31064,
	49572, 28710, 63353, 31327, 11261, 39528, 27727, 53890,
	48327, 60386, 45782, 21502,  7929, 14501, 45681, 12050
};

ALIGNED_AVX2
static const uint16_t NX[] = {
	52463, 12050, 18832, 45681, 50012, 14501, 56584,  7929,
	43011, 21502, 18731, 45782,  4127, 60386, 16186, 48327,
	10623, 53890, 36786, 27727, 24985, 39528, 53252, 11261,
	33186, 31327,  1160, 63353, 35803, 28710, 14941, 49572,
	33449, 31064, 29563, 34950, 58600,  5913,  5322, 59191,
	58637,  5876, 35074, 29439,  2844, 61669, 48108, 16405,
	30362, 34151, 21442, 43071, 17671, 46842,  9560, 54953,
	18586, 45927,  9522, 54991, 54639,  9874, 40669, 23844,
	 3761, 60752, 54909,  9604, 44160, 20353, 44700, 19813,
	 7814, 56699, 11591, 52922, 51816, 12697, 32114, 32399,
	  598, 63915, 44958, 19555, 16267, 48246, 47057, 17456,
	34571, 29942, 59975,  4538, 15546, 48967,   835, 63678,
	49003, 15510, 57754,  6759, 22131, 42382, 35462, 29051,
	35445, 29068, 16507, 48006, 59171,  5342, 54723,  9790,
	33161, 31352, 12442, 52071, 46882, 17631, 62707,  1806,
	60543,  3970, 36828, 27685, 56202,  8311, 63025,  1488
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

				d[j + t] = mq_montymul(mq_sub(u, v), s);

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
		ni = mq_montyred(57083 << (10 - logn));
	} else {
		ni = 7672;
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

			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(mq_montymul(a1, b1), NX[u >> 1]));
			d[u + 1] = mq_add(
				mq_montymul(a1, b0),
				mq_montymul(a0, b1));

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

			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(x,
					mq_add(
						mq_add(
							mq_montymul(a1, b3),
							mq_montymul(a2, b2)),
						mq_montymul(a3, b1))));
			d[u + 1] = mq_add(
				mq_add(
					mq_montymul(a0, b1),
					mq_montymul(a1, b0)),
				mq_montymul(
					x,
					mq_add(
						mq_montymul(a2, b3),
						mq_montymul(a3, b2))));
			d[u + 2] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b2),
						mq_montymul(a1, b1)),
					mq_montymul(a2, b0)),
				mq_montymul(x, mq_montymul(a3, b3)));
			d[u + 3] = mq_add(
				mq_add(
					mq_montymul(a0, b3),
					mq_montymul(a1, b2)),
				mq_add(
					mq_montymul(a2, b1),
					mq_montymul(a3, b0)));

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

			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(
							mq_montymul(a1, b7),
							mq_montymul(a2, b6)),
						mq_add(
							mq_montymul(a3, b5),
							mq_montymul(a4, b4))),
					mq_add(
						mq_add(
							mq_montymul(a5, b3),
							mq_montymul(a6, b2)),
						mq_montymul(a7, b1)))));
			d[u + 1] = mq_add(
				mq_add(
					mq_montymul(a0, b1),
					mq_montymul(a1, b0)),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(
							mq_montymul(a2, b7),
							mq_montymul(a3, b6)),
						mq_add(
							mq_montymul(a4, b5),
							mq_montymul(a5, b4))),
					mq_add(
						mq_montymul(a6, b3),
						mq_montymul(a7, b2)))));
			d[u + 2] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b2),
						mq_montymul(a1, b1)),
					mq_montymul(a2, b0)),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(
							mq_montymul(a3, b7),
							mq_montymul(a4, b6)),
						mq_add(
							mq_montymul(a5, b5),
							mq_montymul(a6, b4))),
					mq_montymul(a7, b3))));
			d[u + 3] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b3),
						mq_montymul(a1, b2)),
					mq_add(
						mq_montymul(a2, b1),
						mq_montymul(a3, b0))),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a4, b7),
						mq_montymul(a5, b6)),
					mq_add(
						mq_montymul(a6, b5),
						mq_montymul(a7, b4)))));
			d[u + 4] = mq_add(
				mq_add(
					mq_add(
						mq_add(
							mq_montymul(a0, b4),
							mq_montymul(a1, b3)),
						mq_add(
							mq_montymul(a2, b2),
							mq_montymul(a3, b1))),
					mq_montymul(a4, b0)),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a5, b7),
						mq_montymul(a6, b6)),
					mq_montymul(a7, b5))));
			d[u + 5] = mq_add(
				mq_add(
					mq_add(
						mq_add(
							mq_montymul(a0, b5),
							mq_montymul(a1, b4)),
						mq_add(
							mq_montymul(a2, b3),
							mq_montymul(a3, b2))),
					mq_add(
						mq_montymul(a4, b1),
						mq_montymul(a5, b0))),
				mq_montymul(x, mq_add(
					mq_montymul(a6, b7),
					mq_montymul(a7, b6))));
			d[u + 6] = mq_add(
				mq_add(
					mq_add(
						mq_add(
							mq_montymul(a0, b6),
							mq_montymul(a1, b5)),
						mq_add(
							mq_montymul(a2, b4),
							mq_montymul(a3, b3))),
					mq_add(
						mq_add(
							mq_montymul(a4, b2),
							mq_montymul(a5, b1)),
						mq_montymul(a6, b0))),
				mq_montymul(x, mq_montymul(a7, b7)));
			d[u + 7] = mq_add(
				mq_add(
					mq_add(
						mq_montymul(a0, b7),
						mq_montymul(a1, b6)),
					mq_add(
						mq_montymul(a2, b5),
						mq_montymul(a3, b4))),
				mq_add(
					mq_add(
						mq_montymul(a4, b3),
						mq_montymul(a5, b2)),
					mq_add(
						mq_montymul(a6, b1),
						mq_montymul(a7, b0))));

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
			z &= a[u] - 64513;
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

			c = mq_montymul(a1, a1);
			c = mq_sub(
				mq_montymul(a0, a0),
				mq_montymul(NX[u >> 1], c));

			z &= c - 64513;
			c = mq_inv(c);

			d[u] = mq_montymul(a0, c);
			d[u + 1] = mq_neg(mq_montymul(a1, c));

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


			b0 = mq_add(
				mq_montymul(a0, a0),
				mq_montymul(x, mq_sub(
					mq_montymul(a2, a2),
					mq_mul2(mq_montymul(a1, a3)))));
			b1 = mq_sub(
				mq_mul2(mq_montymul(a0, a2)),
				mq_add(
					mq_montymul(a1, a1),
					mq_montymul(x, mq_montymul(a3, a3))));
			c = mq_inv(mq_sub(
				mq_montymul(b0, b0),
				mq_montymul(x, mq_montymul(b1, b1))));
			z &= c - 64513;
			b0 = mq_montymul(b0, c);
			b1 = mq_neg(mq_montymul(b1, c));

			d[u] = mq_add(
				mq_montymul(a0, b0),
				mq_montymul(x, mq_montymul(a2, b1)));
			d[u + 1] = mq_neg(mq_add(
				mq_montymul(a1, b0),
				mq_montymul(x, mq_montymul(a3, b1))));
			d[u + 2] = mq_add(
				mq_montymul(a2, b0),
				mq_montymul(a0, b1));
			d[u + 3] = mq_neg(mq_add(
				mq_montymul(a3, b0),
				mq_montymul(a1, b1)));

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


			b0 = mq_add(
				mq_montymul(a0, a0),
				mq_montymul(x, mq_add(
					mq_montymul(a4, a4),
					mq_mul2(mq_sub(
						mq_montymul(a2, a6),
						mq_add(
							mq_montymul(a1, a7),
							mq_montymul(a3, a5)))))));
			b1 = mq_add(
				mq_sub(
					mq_mul2(mq_montymul(a0, a2)),
					mq_montymul(a1, a1)),
				mq_montymul(x, mq_sub(
					mq_mul2(mq_sub(
						mq_montymul(a4, a6),
						mq_montymul(a3, a7))),
					mq_montymul(a5, a5))));
			b2 = mq_add(
				mq_add(
					mq_montymul(a2, a2),
					mq_mul2(mq_sub(
						mq_montymul(a0, a4),
						mq_montymul(a1, a3)))),
				mq_montymul(x, mq_sub(
					mq_montymul(a6, a6),
					mq_mul2(mq_montymul(a5, a7)))));
			b3 = mq_sub(
				mq_mul2(mq_sub(
					mq_add(
						mq_montymul(a0, a6),
						mq_montymul(a2, a4)),
					mq_montymul(a1, a5))),
				mq_add(
					mq_montymul(a3, a3),
					mq_montymul(x, mq_montymul(a7, a7))));

			c0 = mq_add(
				mq_montymul(b0, b0),
				mq_montymul(x, mq_sub(
					mq_montymul(b2, b2),
					mq_mul2(mq_montymul(b1, b3)))));
			c1 = mq_sub(
				mq_mul2(mq_montymul(b0, b2)),
				mq_add(
					mq_montymul(b1, b1),
					mq_montymul(x, mq_montymul(b3, b3))));
			e = mq_inv(mq_sub(
				mq_montymul(c0, c0),
				mq_montymul(x, mq_montymul(c1, c1))));
			z &= e - 64513;
			c0 = mq_montymul(c0, e);
			c1 = mq_neg(mq_montymul(c1, e));

			f0 = mq_add(
				mq_montymul(b0, c0),
				mq_montymul(x, mq_montymul(b2, c1)));
			f1 = mq_neg(mq_add(
				mq_montymul(b1, c0),
				mq_montymul(x, mq_montymul(b3, c1))));
			f2 = mq_add(
				mq_montymul(b2, c0),
				mq_montymul(b0, c1));
			f3 = mq_neg(mq_add(
				mq_montymul(b3, c0),
				mq_montymul(b1, c1)));

			d[u] = mq_add(
				mq_montymul(a0, f0),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a2, f3),
						mq_montymul(a4, f2)),
					mq_montymul(a6, f1))));
			d[u + 1] = mq_neg(mq_add(
				mq_montymul(a1, f0),
				mq_montymul(x, mq_add(
					mq_add(
						mq_montymul(a3, f3),
						mq_montymul(a5, f2)),
					mq_montymul(a7, f1)))));
			d[u + 2] = mq_add(
				mq_add(
					mq_montymul(a0, f1),
					mq_montymul(a2, f0)),
				mq_montymul(x, mq_add(
					mq_montymul(a4, f3),
					mq_montymul(a6, f2))));
			d[u + 3] = mq_neg(mq_add(
				mq_add(
					mq_montymul(a1, f1),
					mq_montymul(a3, f0)),
				mq_montymul(x, mq_add(
					mq_montymul(a5, f3),
					mq_montymul(a7, f2)))));
			d[u + 4] = mq_add(
				mq_add(
					mq_montymul(a0, f2),
					mq_montymul(a2, f1)),
				mq_add(
					mq_montymul(a4, f0),
					mq_montymul(x, mq_montymul(a6, f3))));
			d[u + 5] = mq_neg(mq_add(
				mq_add(
					mq_montymul(a1, f2),
					mq_montymul(a3, f1)),
				mq_add(
					mq_montymul(a5, f0),
					mq_montymul(x, mq_montymul(a7, f3)))));
			d[u + 6] = mq_add(
				mq_add(
					mq_montymul(a0, f3),
					mq_montymul(a2, f2)),
				mq_add(
					mq_montymul(a4, f1),
					mq_montymul(a6, f0)));
			d[u + 7] = mq_neg(mq_add(
				mq_add(
					mq_montymul(a1, f3),
					mq_montymul(a3, f2)),
				mq_add(
					mq_montymul(a5, f1),
					mq_montymul(a7, f0))));

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
	51870, 41285
};

static const uint16_t TT2[] = {
	57594, 46146, 47009, 35561
};

static const uint16_t TT3[] = {
	17815, 32860, 20468,  7311, 21331,  8174, 60295, 10827
};

static const uint16_t TT4[] = {
	50374, 49769, 28753, 36967, 35100,  5836, 59024, 20111,
	 8531, 34131, 22806, 58055, 56188, 64402, 43386, 42781
};

static const uint16_t TT5[] = {
	63672, 37076, 51977, 47561, 16345, 41161, 55429, 18505,
	 4032,  1655,  8808,  2864, 49976,  3559, 31777,  8445,
	20197, 61378, 25083, 43179, 25778, 19834, 26987, 24610,
	10137, 37726, 51994, 12297, 45594, 41178, 56079, 29483
};

static const uint16_t TT6[] = {
	12126, 50705, 11707, 62445, 49627, 54327, 59852, 35270,
	17310, 15380, 16703,  1106, 27633, 18712, 23743, 13267,
	 3682,  4382, 45431, 22392, 41204, 40925,  4775,   953,
	44949, 55003, 49689, 21942, 18267, 45287, 28338, 53065,
	40090,   304, 47868, 10375,  6700, 43466, 38152, 48206,
	27689, 23867, 52230, 51951,  6250, 47724, 24260, 24960,
	15375,  4899,  9930,  1009, 27536, 11939, 13262, 11332,
	57885, 33303, 38828, 43528, 30710, 16935, 42450, 16516
};

static const uint16_t TT7[] = {
	  585, 23667, 32462,  4435, 60735, 27192, 42895, 17482,
	50967, 48287, 45874, 62780, 44098, 11093,  4354,  1673,
	13505, 21115, 18884, 11876,  9364, 24042, 53145, 13580,
	59318, 60461,  1231, 36193, 43707,  3779, 57840, 33207,
	52870, 19007, 29145, 44132, 59648, 31214, 32727, 12057,
	37267, 45141, 39280, 42570, 46442, 27621, 59365,  7054,
	  836, 24549, 14177, 31316, 16482, 18383, 16899, 26985,
	59232, 41815, 10205, 15856, 24715, 31961, 44768, 61362,
	31793, 48387, 61194,  3927, 12786, 18437, 51340, 33923,
	 1657, 11743, 10259, 12160, 61839, 14465,  4093, 27806,
	21588, 33790,  1021, 46713, 50585, 53875, 48014, 55888,
	16585, 60428, 61941, 33507, 49023, 64010,  9635, 40285,
	59948, 35315, 24863, 49448, 56962, 27411, 32694, 33837,
	15062, 40010,  4600, 19278, 16766,  9758,  7527, 15137,
	26969, 24288, 17549, 49057, 30375, 47281, 44868, 42188,
	11160, 50260,  1450, 32420, 24207, 60693,  4975, 28057
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

			d[u] = mq_montymul(b, mq_add(a0,
				mq_montymul(a1, NX[u >> 1])));
			d[u + 1] = mq_montymul(b, mq_add(a0, a1));

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

			d[u] = mq_montymul(b, mq_add(a0, mq_montymul(x,
				mq_add(mq_add(a1, a2), a3))));
			d[u + 1] = mq_montymul(b, mq_add(
				mq_add(a0, a1),
				mq_montymul(x, mq_add(a2, a3))));
			d[u + 2] = mq_montymul(b, mq_add(
				mq_add(a0, a1),
				mq_add(a2, mq_montymul(x, a3))));
			d[u + 3] = mq_montymul(b, mq_add(
				mq_add(a0, a1), mq_add(a2, a3)));

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

			d[u] = mq_montymul(b, mq_add(a0, mq_montymul(x,
				mq_add(
					mq_add(
						mq_add(a1, a2),
						mq_add(a3, a4)),
					mq_add(
						mq_add(a5, a6),
						a7)))));
			d[u + 1] = mq_montymul(b, mq_add(
				mq_add(a0, a1),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(a2, a3),
						mq_add(a4, a5)),
					mq_add(a6, a7)))));
			d[u + 2] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(a0, a1),
					a2),
				mq_montymul(x, mq_add(
					mq_add(
						mq_add(a3, a4),
						mq_add(a5, a6)),
					a7))));
			d[u + 3] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(a0, a1),
					mq_add(a2, a3)),
				mq_montymul(x, mq_add(
					mq_add(a4, a5),
					mq_add(a6, a7)))));
			d[u + 4] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(
						mq_add(a0, a1),
						mq_add(a2, a3)),
					a4),
				mq_montymul(x, mq_add(
					mq_add(a5, a6),
					a7))));
			d[u + 5] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(
						mq_add(a0, a1),
						mq_add(a2, a3)),
					mq_add(a4, a5)),
				mq_montymul(x, mq_add(a6, a7))));
			d[u + 6] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(
						mq_add(a0, a1),
						mq_add(a2, a3)),
					mq_add(
						mq_add(a4, a5),
						a6)),
				mq_montymul(x, a7)));
			d[u + 7] = mq_montymul(b, mq_add(
				mq_add(
					mq_add(a0, a1),
					mq_add(a2, a3)),
				mq_add(
					mq_add(a4, a5),
					mq_add(a6, a7))));

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

