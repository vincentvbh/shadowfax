
/*
 * BAT key pair generation.
 *
 * =====================================================================
 * WARNING: This file was derived from the key pair generation in
 * Falcon. Falcon uses a different naming convention: in BAT, the
 * NTRU equation is:
 *    g*F - f*G = q
 * whereas the Falcon convention is:
 *    f*G - g*F = q
 * i.e. the Falcon convention exchanges the roles of f and g, and
 * F and G. Throughout most of this file, the Falcon convention is used.
 * The exchange is done in the public functions at the end of this file
 * (starting at bat_keygen_make_fg()).
 * =====================================================================
 */

#include "encode_decode.h"
#include "rng.h"

#include "kem128.h"
#include "kem257.h"
#include "kem769.h"
#include "keygen.h"

#include "ng_fxp.h"
#include "ng_fft.h"
#include "ng_mp31.h"
#include "ng_ntru.h"

/* ==================================================================== */

/*
 * Table gauss_n_q incarnates a discrete Gaussian distribution:
 *    D(x) = exp(-(x^2)/(2*sigma^2))
 * where sigma = 2^(1/4)*sqrt(q/(2*N)).
 *
 * Each table contains elements for indices -kmax to +kmax-1 (inclusive)
 * (kmax is the maximum absolute value of sample outputs, given our
 * 64-bit precision; this is 5 or 6 for the tables below). At index k,
 * the table contains P(x <= k), scaled up by a factor 2^64.
 */

static const uint64_t gauss_256_128[] = {
	                5459u,           1840064364u,       36656378137925u,
	   43193878477851778u,  3046564455360581672u, 15400179618348969942u,
	18403550195231699836u, 18446707417331413689u, 18446744071869487250u,
	18446744073709546155u
};

static const uint64_t gauss_512_257[] = {
	                6252u,           2005605095u,       38444478492594u,
	   44072422491847837u,  3058285043251155898u, 15388459030458395716u,
	18402671651217703777u, 18446705629231059020u, 18446744071703946519u,
	18446744073709545362u
};

static const uint64_t gauss_1024_769[] = {
	                  11u,              3660696u,         114233193962u,
	     357617305475568u,   112576638291761591u,  3645534795308962022u,
	14801209278400589592u, 18334167435417790023u, 18446386456404076046u,
	18446743959476357652u, 18446744073705890918u, 18446744073709551603u
};

/*
 * Given a uniformly random 64-bit integer (0 to 2^64-1), sample a value
 * using the provided Gaussian distribution table. Returned value is
 * in the -kmax..+kmax range. The function reads all 2*kmax entries of the
 * table.
 */
static inline int
gauss_sample(const uint64_t *tab, unsigned kmax, uint64_t x)
{
	/*
	 * Sampling algorithm: given x (in 0..2^64-1), we count the
	 * number v of table elements that are lower than or equal to x.
	 * All table elements are looked up. Output is v-kmax.
	 *
	 * Table elements happen to be sorted in increasing order, but
	 * this property is not used (it could be leveraged for a
	 * binary search, but this would not be constant-time).
	 *
	 * To do a constant-time comparison of x and y:
	 *  - If both x < 2^63 and y < 2^63, then y-x has its
	 *    high bit set iff x > y.
	 *  - If either x >= 2^63 or y >= 2^63 (but not both),
	 *    then x > y iff the high bit of x is 1.
	 *  - If both x >= 2^63 and y >= 2^63, then
	 *    y-x = (y - 2^63) - (x - 2^63), and we are back
	 *    to the first case.
	 */
	unsigned u, v;

	v = 0;
	for (u = 0; u < (2 * kmax); u ++) {
		uint64_t y, z;

		y = tab[u];
		z = y - x;
		v += (unsigned)((z ^ ((x ^ y) & (x ^ z))) >> 63);
	}
	return (int)v - (int)kmax;
}

/*
 * Generate a random polynomial with a Gaussian distribution centered
 * on 0. The RNG must be ready for extraction (already flipped). The
 * distribution depends on the value q, which must be one of the
 * supported values (128, 257 or 769). Maximum value of logn depends on q:
 *   q = 128:   max logn = 8
 *   q = 257:   max logn = 9
 *   q = 769:   max logn = 10
 *
 * When the degree is less than the maximum, several values are generated
 * and added together (the standard deviation is in sqrt(q/(2*n)), so this
 * computes things with the right distribution).
 *
 * Returned value is 1 on success, 0 on failure. A failure is reported if
 * any value is outside of the -lim..+lim range ('lim' must be at most 127).
 *
 * Generation is constant-time on success (if a failure is reported, the
 * whole seed should be abandoned anyway).
 */
static int
mkgauss(prng *rng, int8_t *f, uint32_t q, unsigned logn, int lim)
{
	size_t u, n;
	unsigned g;
	const uint64_t *tab;
	unsigned kmax;

	switch (q) {
	case 128:
		g = (size_t)1 << (8 - logn);
		tab = gauss_256_128;
		kmax = 5;
		break;
	case 257:
		g = (size_t)1 << (9 - logn);
		tab = gauss_512_257;
		kmax = 5;
		break;
	case 769:
		g = (size_t)1 << (10 - logn);
		tab = gauss_1024_769;
		kmax = 6;
		break;
	default:
		/* impossible in practice */
		return 0;
	}

	n = (size_t)1 << logn;
	for (u = 0; u < n; u ++) {
		int val;
		unsigned v;

		val = 0;
		for (v = 0; v < g; v ++) {
			val += gauss_sample(tab, kmax, prng_get_u64(rng));
		}
		if (val < -lim || val > +lim) {
			return 0;
		}
		f[u] = (int8_t)val;
	}
	return 1;
}

/*
 * Compute squared norm of a short vector. Maximum possible returned value
 * is 16384*(2^logn), which will fit in a uint32_t as long as logn <= 17
 * (the rest of this implementation assumes that logn <= 10).
 */
static uint32_t
poly_small_sqnorm(const int8_t *f, unsigned logn)
{
	size_t n, u;
	uint32_t s;

	n = (size_t)1 << logn;
	s = 0;
	for (u = 0; u < n; u ++) {
		int32_t z;

		z = f[u];
		s += (uint32_t)(z * z);
	}
	return s;
}

/*
 * Generate a random polynomial with a Gaussian distribution. This function
 * reports a failure if mkgauss() failed (one output was out of range)
 * or if the resulting polynomial has even parity (i.e. its resultant
 * with X^n+1 is an even integer).
 *
 * Values q and logn MUST be a supported combination.
 */
static int
poly_small_mkgauss(prng *rng, int8_t *f, uint32_t q, unsigned logn)
{
	size_t u, n;
	unsigned mod2;

	if (!mkgauss(rng, f, q, logn, 1 << (bat_max_fg_bits[logn] - 1))) {
		return 0;
	}
	n = (size_t)1 << logn;
	mod2 = 0;
	for (u = 0; u < n; u ++) {
		mod2 ^= (unsigned)f[u];
	}
	return (int)(mod2 & 1u);
}

/*
 * Compute the vector w:
 *   w = round(qp*(gamma2*G*adj(g)+F*adj(f))/(gamma2*g*adj(g)+f*adj(f)))
 *
 * Value qp is normally 64513, and gamma2 = (k^2-1)/3 = 1 or 5. All
 * coefficients of w are supposed to fit on signed 17-bit integers (all
 * values should be in -65535..+65535).
 *
 * Return value: 1 on success, 0 on error (an error is reported on overflow
 * of any coefficient of w).
 */
static int
compute_w(int32_t *w, const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G, unsigned qp, unsigned gamma2,
	unsigned logn, uint32_t *tmp)
{

	size_t n = (size_t)1 << logn;
	size_t hn = n >> 1;

	/*
	 * Memory layout:
	 *   gm    NTT support (n)
	 *   Ft    receives F (n)
	 *   Gt    receives G (n)
	 *   ft    receives f (n)
	 *   gt    receives g (n)
	 */
	uint32_t *gm = tmp;
	uint32_t *Ft = gm + n;
	uint32_t *Gt = Ft + n;
	uint32_t *ft = Gt + n;
	uint32_t *gt = ft + n;

	/*
	 * Load polynomials and convert to NTT.
	 */
	uint32_t p = PRIMES[0].p;
	uint32_t p0i = PRIMES[0].p0i;
	uint32_t R2 = PRIMES[0].R2;
	mp_mkgm(logn, gm, PRIMES[0].g, p, p0i);
	for (size_t u = 0; u < n; u ++) {
		Ft[u] = mp_set(F[u], p);
		Gt[u] = mp_set(G[u], p);
		ft[u] = mp_set(f[u], p);
		gt[u] = mp_set(g[u], p);
	}
	mp_NTT(logn, Ft, gm, p, p0i);
	mp_NTT(logn, Gt, gm, p, p0i);
	mp_NTT(logn, ft, gm, p, p0i);
	mp_NTT(logn, gt, gm, p, p0i);

	/*
	 * gmv <- R*R*gamma^2
	 */
	uint32_t gmv = mp_montymul(R2, mp_montymul(R2,
		mp_set(gamma2, p), p, p0i), p, p0i);

	/*
	 * Compute F*adj(f) + (gamma^2)*G*adj(g) into t1 (RNS+NTT).
	 * t1 is an alias on gm.
	 */
	uint32_t *t1 = gm;
	for (size_t u = 0; u < n; u ++) {
		uint32_t xF = Ft[u];
		uint32_t xG = Gt[u];
		uint32_t xfa = mp_montymul(gmv, ft[(n - 1) - u], p, p0i);
		uint32_t xga = mp_montymul(R2, gt[(n - 1) - u], p, p0i);
		t1[u] = mp_add(
			mp_montymul(xF, xfa, p, p0i),
			mp_montymul(xG, xga, p, p0i), p);
	}

	/*
	 * Compute (gamma^2)*f*adj(f) + g*adj(g) into t2 (plain, 32-bit).
	 * t2 is an alias on Ft.
	 */
	uint32_t *t2 = Ft;
	for (size_t u = 0; u < n; u ++) {
		uint32_t xf = ft[u];
		uint32_t xg = gt[u];
		uint32_t xfa = mp_montymul(gmv, ft[(n - 1) - u], p, p0i);
		uint32_t xga = mp_montymul(R2, gt[(n - 1) - u], p, p0i);
		t2[u] = mp_add(
			mp_montymul(xf, xfa, p, p0i),
			mp_montymul(xg, xga, p, p0i), p);
	}

	/*
	 * Convert t1 and t2 to plain (32-bit).
	 */
	uint32_t *igm = t2 + n;
	mp_mkigm(logn, igm, PRIMES[0].ig, p, p0i);
	mp_iNTT(logn, t1, igm, p, p0i);
	mp_iNTT(logn, t2, igm, p, p0i);
	for (size_t u = 0; u < n; u ++) {
		t1[u] = (uint32_t)mp_norm(t1[u], p);
		t2[u] = (uint32_t)mp_norm(t2[u], p);
	}

	/*
	 * For the division, we go into the FFT domain. We check that the
	 * FFT won't overflow. The dividend is scaled down by 10 bits to
	 * compensate for the multiplication by q'.
	 *
	 * Buffer reorganization:
	 *    t1    (gamma^2)*F*adj(f) + G*adj(g) (plain, 32-bit) (n)
	 *    t2    (gamma^2)*f*adj(f) + g*adj(g) (plain, 32-bit) (n)
	 *    rt1   receives the dividend (FFT) (n fxr = 2*n)
	 *    rt2   receives the divisor (FFT) (hn fxr = n)
	 *
	 * The divisor is auto-adjoint, so it uses only half of the
	 * space in FFT representation; we compute in rt1 then move it
	 * to rt2.
	 */
	fxr *rt1 = (fxr *)(t2 + n);
	fxr *rt2 = (fxr *)(rt1 + n);
	for (size_t u = 0; u < n; u ++) {
		rt1[u] = fxr_of(*(int32_t *)&t2[u]);
	}
	vect_FFT(logn, rt1);
	memmove(rt2, rt1, hn * sizeof *rt1);

	/*
	 * For the dividend, we multiply by q' but also scale down
	 * by 2^10; we check that the operation won't overflow.
	 */
	int32_t lim1 = (int32_t)(((uint64_t)1 << (41 - logn)) / qp);
	for (size_t u = 0; u < n; u ++) {
		int32_t x = *(int32_t *)&t1[u];
		if (x <= -lim1 || x >= +lim1) {
			return 0;
		}
		rt1[u] = fxr_of_scaled32(((uint64_t)x * qp) << 22);
	}
	vect_FFT(logn, rt1);

	/*
	 * Divisor is auto-adjoint. We inline the division loop here because
	 * we also want to check on overflows.
	 */
	for (size_t u = 0; u < hn; u ++) {
		fxr z1r = rt1[u];
		fxr z1i = rt1[u + hn];
		fxr z2 = rt2[u];
		if (!fxr_lt(fxr_div2e(fxr_abs(z1r), 30 - logn), z2)
			|| !fxr_lt(fxr_div2e(fxr_abs(z1i), 30 - logn), z2))
		{
			return 0;
		}
		rt1[u] = fxr_div(z1r, z2);
		rt1[u + hn] = fxr_div(z1i, z2);
	}
	vect_iFFT(logn, rt1);

	/*
	 * The unrounded w is in rt1 (scaled down by 2^10); we just have to
	 * round the coefficients and check that they are all in the
	 * allowed [-2^16..+2^16] range.
	 */
	t1 = tmp;
	fxr lim2 = fxr_of(1 << 6);
	for (size_t u = 0; u < n; u ++) {
		if (fxr_lt(lim2, fxr_abs(rt1[u]))) {
			return 0;
		}
		w[u] = (uint32_t)fxr_round(fxr_mul2e(rt1[u], 10));
	}

	return 1;
}


/*
 * Compute the squared norm of (Fd, gamma*Gd) with:
 *   Fd = qp*F - f*w
 *   Gd = qp*G - g*w
 */
static uint64_t
compute_dnorm(const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G, const int32_t *w,
	unsigned qp, unsigned gamma2, unsigned logn, uint32_t *tmp)
{
	size_t n, u;
	uint32_t *gm, *igm, *t1, *t2, *t3;
	uint32_t p, p0i, R2;
	uint64_t dnorm1, dnorm2;

	/*
	 * Max values for f and g is 6*(1024/n) (in absolute value);
	 * coefficients of F and G are less than 2^7, and coefficients
	 * of w are less than 2^16. Thus, maximum absolute value of a
	 * coefficient of Fd or Gd is n*6*(1024/n)*(2^16-1) +
	 * qp*(2^7-1). With qp = 64513, this is 410840191. Thus, we
	 * can compute Fd and Gd with the NTT and a single prime, as long
	 * as that prime is greater than 821680383.
	 */
	n = (size_t)1 << logn;
	gm = tmp;
	igm = gm + n;
	t1 = igm + n;
	t2 = t1 + n;
	t3 = t2 + n;

	p = PRIMES[0].p;
	p0i = PRIMES[0].p0i;
	R2 = PRIMES[0].R2;
	mp_mkgmigm(logn, gm, igm, PRIMES[0].g, PRIMES[0].ig, p, p0i);


	for (u = 0; u < n; u ++) {
		t1[u] = mp_montymul(R2, mp_set(*(int32_t *)&w[u], p), p, p0i);
	}
	mp_NTT(logn, t1, gm, p, p0i);


	/*
	 * Load f in t2 and qp*F in t3; convert both to NTT, compute Fd,
	 * and compute its squared norm.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = mp_set(f[u], p);
		t3[u] = mp_set((int32_t)qp * (int32_t)F[u], p);
	}

	mp_NTT(logn, t2, gm, p, p0i);
	mp_NTT(logn, t3, gm, p, p0i);
	for (u = 0; u < n; u++){
		t2[u] = mp_sub(t3[u], mp_montymul(t1[u], t2[u], p, p0i), p);
	}
	mp_iNTT(logn, t2, igm, p, p0i);
	dnorm1 = 0;
	for (u = 0; u < n; u ++) {
		int32_t x;

		x = mp_norm(t2[u], p);
		dnorm1 += (uint64_t)((int64_t)x * (int64_t)x);
	}

	/*
	 * Load g in t2 and qp*G in t3; convert both to NTT, compute Gd,
	 * and compute its squared norm.
	 */
	for (u = 0; u < n; u ++) {
		t2[u] = mp_set(g[u], p);
		t3[u] = mp_set((int32_t)qp * (int32_t)G[u], p);
	}
	mp_NTT(logn, t2, gm, p, p0i);
	mp_NTT(logn, t3, gm, p, p0i);
	for (u = 0; u < n; u++){
		t2[u] = mp_sub(t3[u], mp_montymul(t1[u], t2[u], p, p0i), p);
	}
	mp_iNTT(logn, t2, igm, p, p0i);
	dnorm2 = 0;
	for (u = 0; u < n; u ++) {
		int32_t x;

		x = mp_norm(t2[u], p);
		dnorm2 += (uint64_t)((int64_t)x * (int64_t)x);
	}

	/*
	 * Result is dnorm1 + (gamma^2)*dnorm2, since there is a factor
	 * gamma on Gd.
	 */
	return dnorm1 + (uint64_t)gamma2 * dnorm2;
}

static const uint64_t max_dnorm_128[] = {
	753412648927,
	753436005671,
	753482720245,
	753576153737,
	753763038100,
	754136876336,
	754884830854,
	756381852074,
};

static const uint64_t max_dnorm_257[] = {
	1512711334174,
	1512758230136,
	1512852024242,
	1513039621175,
	1513414849934,
	1514165447018,
	1515667199450,
	1518672937367,
	1524693345421,
};

static const uint64_t max_dnorm_769[] = {
	11119929273450,
	11120274005305,
	11120963485046,
	11122342508650,
	11125100812344,
	11130618445681,
	11141657816146,
	11163752972243,
	11208008945103,
	11296783533487,
};

/* see keygen.h */
int
bat_keygen_compute_w(int32_t *w, const int8_t *f, const int8_t *g,
	const int8_t *F, const int8_t *G, uint32_t q, unsigned logn,
	uint32_t *tmp)
{
	int gamma2;
	uint64_t dnorm, max_dnorm;

	switch (q) {
	case 128:
		if (logn == 0 || logn > 8) {
			return 0;
		}
		gamma2 = 1;
		max_dnorm = max_dnorm_128[logn - 1];
		break;
	case 257:
		if (logn == 0 || logn > 9) {
			return 0;
		}
		gamma2 = 1;
		max_dnorm = max_dnorm_257[logn - 1];
		break;
	case 769:
		if (logn == 0 || logn > 10) {
			return 0;
		}
		gamma2 = 5;
		max_dnorm = max_dnorm_769[logn - 1];
		break;
	default:
		return 0;
	}

	/*
	 * Compute w. Beware that compute_w() uses the Falcon convention.
	 */
	if (!compute_w(w, g, f, G, F, 64513, gamma2, logn, tmp)) {
		return 0;
	}

	/*
	 * Verify that (gamma*Fd, Gd) is small enough.
	 */
	dnorm = compute_dnorm(g, f, G, F, w, 64513, gamma2, logn, tmp);
	if (dnorm > max_dnorm) {
		return 0;
	}

	return 1;
}

/* ===================================================================== */
/*
 * Functions below this line use the BAT naming conventions (g*F - f*G = q).
 */

/* see keygen.h */
int
bat_keygen_make_fg(int8_t *f, int8_t *g, uint16_t *h,
	uint32_t q, unsigned logn,
	const void *seed, size_t seed_len, uint32_t *tmp)
{
	size_t n;
	prng rng;
	uint32_t normf, normg, gamma2, bound_norm2_fg;

	n = (size_t)1 << logn;

	switch (q) {
	case 128:
		if (logn == 0 || logn > 8) {
			return 0;
		}
		gamma2 = 1;
		bound_norm2_fg = 181;
		break;
	case 257:
		if (logn == 0 || logn > 9) {
			return 0;
		}
		gamma2 = 1;
		bound_norm2_fg = 363;
		break;
	case 769:
		if (logn == 0 || logn > 10) {
			return 0;
		}
		gamma2 = 5;
		bound_norm2_fg = 2671;
		break;
	default:
		return 0;
	}

	/*
	 * The BLAKE2s-based PRNG is initialized with the provided seed;
	 * the 'label' contains the modulus (q, 16-bit) and the logarithm
	 * of the degree (logn, in bits 16..19).
	 */
	prng_init(&rng, seed, seed_len, q | (uint32_t)logn << 16);

	/*
	 * Generate f and g.
	 */
	if (!poly_small_mkgauss(&rng, f, q, logn)
		|| !poly_small_mkgauss(&rng, g, q, logn))
	{
		return 0;
	}

	/*
	 * Bound on the norm of (g, gamma*f) is:
	 *    sqrt(n) * sigma * sqrt(gamma^2 + 1)
	 * Since the expression of sigma includes 1/sqrt(n), the value
	 * of n simplifies away; i.e. the bound depends on q and gamma,
	 * but not on n.
	 */
	normf = poly_small_sqnorm(f, logn);
	normg = poly_small_sqnorm(g, logn);
	if (normg + gamma2 * normf > bound_norm2_fg) {
		return 0;
	}

	/*
	 * Compute public key h = g/f mod X^n+1 mod q. We must do it
	 * even if the caller did not ask for it, because we must report
	 * a failure if f is not invertible modulo X^n+1 modulo q.
	 */
	if (h == NULL) {
		h = (uint16_t *)tmp;
		tmp = (uint32_t *)(h + n);
	}
	switch (q) {
	case 128:
		if (!bat_make_public_128((uint8_t *)h, f, g, logn, tmp)) {
			return 0;
		}
		break;
	case 257:
		if (!bat_make_public_257(h, f, g, logn, tmp)) {
			return 0;
		}
		break;
	case 769:
		if (!bat_make_public_769(h, f, g, logn, tmp)) {
			return 0;
		}
		break;
	}

	return 1;
}

/* see keygen.h */
int
bat_keygen_solve_FG(int8_t *F, int8_t *G, const int8_t *f, const int8_t *g,
	uint32_t q, unsigned logn, uint32_t *tmp)
{

	/*
	 * Solve the NTRU equation to get F and G.
	 * (caution: solve_NTRU() expects values in Falcon order, hence
	 * the swap (f,g) <-> (g,f) and (F,G) <-> (G,F))
	 */
	switch (q) {
	case 128: if (logn == 0 || logn > 8) { return 0; } break;
	case 257: if (logn == 0 || logn > 9) { return 0; } break;
	case 769: if (logn == 0 || logn > 10) { return 0; } break;
	default:
		return 0;
	}

	switch (q){
		case 128: return solve_NTRU(&SOLVE_BAT_128_256, logn, g, f, G, F, tmp) == SOLVE_OK;
		case 257: return solve_NTRU(&SOLVE_BAT_257_512, logn, g, f, G, F, tmp) == SOLVE_OK;
		case 769: return solve_NTRU(&SOLVE_BAT_769_1024, logn, g, f, G, F, tmp) == SOLVE_OK;
	default:
		return 0;
	}

}

/* see keygen.h */
int
bat_keygen_rebuild_G(int8_t *G,
	const int8_t *f, const int8_t *g, const int8_t *F,
	uint32_t q, unsigned logn, uint32_t *tmp)
{
	/*
	 * Since g*F - f*G = q, and f is invertible modulo q, we can
	 * recompute G modulo q with:
	 *   G = (g*F)/f  mod X^n+1 mod q
	 * The coefficients of G are supposed to be small (they must fit
	 * in -127..+127), hence G itself can be obtained from G modulo q.
	 */
	switch (q) {

	case 128:
		if (logn == 0 || logn > 8) {
			return 0;
		}
		return bat_rebuild_G_128(G, f, g, F, logn, tmp);

	case 257:
		if (logn == 0 || logn > 9) {
			return 0;
		}
		return bat_rebuild_G_257(G, f, g, F, logn, tmp);

	case 769:
		if (logn == 0 || logn > 10) {
			return 0;
		}
		return bat_rebuild_G_769(G, f, g, F, logn, tmp);

	default:
		return 0;
	}
}




