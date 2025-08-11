/*
 * Core signature generation.
 */

#include "sign_inner.h"

/* Given f, g, F and G, return the basis [[g, -f], [G, -F]] in FFT
   format (b00, b01, b10 and b11, in that order, are written in the
   destination). */
static void
basis_to_FFT(unsigned logn,
	fpr *b00, fpr *b01, fpr *b10, fpr *b11,
	const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G)
{
	size_t n = (size_t)1 << logn;
	fpoly_set_small(logn, b01, f);
	fpoly_set_small(logn, b00, g);
	fpoly_set_small(logn, b11, F);
	fpoly_set_small(logn, b10, G);
	fpoly_FFT(logn, b01);
	fpoly_FFT(logn, b00);
	fpoly_FFT(logn, b11);
	fpoly_FFT(logn, b10);
	fpoly_neg(logn, b01);
	fpoly_neg(logn, b11);
}

/* see sign_inner.h */
TARGET_SSE2 TARGET_NEON
size_t
sign_core(unsigned logn,
	const uint8_t *sign_key_fgF, const int8_t *G,
	const uint8_t *hashed_vk, const uint8_t *ctx, size_t ctx_len,
	const char *id, const uint8_t *hv, size_t hv_len,
	const uint8_t *seed, size_t seed_len, uint8_t *sig, void *tmp)
{
	/* Output value is 0 on error, or the signature length on success. */
	size_t ret = 0;

#if FNDSA_SSE2
	/* We must ensure that the rounding mode is appropriate (we need
	   the roundTiesToEven policy, which is normally the default, but
	   could have been changed by the calling application. */
	unsigned round_mode = _MM_GET_ROUNDING_MODE();
	_MM_SET_ROUNDING_MODE(_MM_ROUND_NEAREST);
#endif
	/* Note: on aarch64, we use only NEON intrinsics, and NEON always
	   uses "round to nearest" mode, which is the IEEE-754
	   roundTiesToEven (NEON also always flushes denormals to zero,
	   which is not IEEE-754 compliant, but we do not care because
	   we never get any denormal value anywhere in signature
	   generation). */

	size_t n = (size_t)1 << logn;
	size_t hn = n >> 1;
	unsigned nbits;
	switch (logn) {
	case 2: case 3: case 4: case 5:
		nbits = 8;
		break;
	case 6: case 7:
		nbits = 7;
		break;
	case 8: case 9:
		nbits = 6;
		break;
	default:
		nbits = 5;
		break;
	}
	/* Storage format of F is already an array of int8_t. */
	size_t flen = (nbits << logn) >> 3;
	int8_t *F = (int8_t *)(sign_key_fgF + (flen << 1));

	/* The special hash identifier consisting of a byte of value 0xFF
	   followed by a zero denotes the original Falcon algorithm, which
	   we support for now for test vector reproducibility.
	   TODO: remove when final test vectors are available. */
	int orig_falcon = (*(const uint8_t *)id == 0xFF && id[1] == 0);

	/* Buffer for randomness: we need a 40-byte nonce and a 56-byte
	   sub-seed for sampling. */
	uint8_t rndbuf[40 + 56];
	uint8_t *nonce = rndbuf;
	uint8_t *subseed = rndbuf + 40;

	/* TODO: add "hedging" by hashing together the private key, the
	   message and the seed, to get a new seed value (protection against
	   bad RNG). We don't need to use the complete private key; we can
	   use F. */

	for (uint32_t counter = 0;; counter ++) {
		/* Generate the nonce and the sub-seed. In the original
		   Falcon, the nonce was not regenerated in case of
		   restart, but regenerating it makes the algorithm security
		   easier to analyze and prove.

		   When working with an explicit seed: normally, we hash
		   together the seed and a loop counter. For test purposes,
		   if we are using the original Falcon behaviour, this is
		   the first iteration, and the seed length is exactly 96
		   bytes, then we use the seed directly. */
		uint8_t *rndp;
		size_t rndlen;
		if (counter == 0 || !orig_falcon) {
			rndp = rndbuf;
			rndlen = sizeof rndbuf;
		} else {
			rndp = rndbuf + 40;
			rndlen = (sizeof rndbuf) - 40;
		}
		if (seed == NULL) {
			if (!sysrng(rndp, rndlen)) {
				goto sign_exit;
			}
		} else if (orig_falcon && counter == 0 && seed_len == rndlen) {
			memcpy(rndp, seed, rndlen);
		} else {
			/* We can use the tmp buffer for the SHAKE context.
			   It even works at n = 4 (logn = 2) because there
			   are 58*4 = 232 bytes free in tmp[] at this point,
			   and we need only 208. */
			shake_context *sc = (shake_context *)tmp;
			shake_init(sc, 256);
			shake_inject(sc, seed, seed_len);
			uint8_t cbuf[4];
			cbuf[0] = (uint8_t)counter;
			cbuf[1] = (uint8_t)(counter >> 8);
			cbuf[2] = (uint8_t)(counter >> 16);
			cbuf[3] = (uint8_t)(counter >> 24);
			shake_inject(sc, cbuf, 4);
			shake_flip(sc);
			shake_extract(sc, rndp, rndlen);
		}

		/* Hash the message into a polynomial. */
		uint16_t *hm = (uint16_t *)((uint8_t *)tmp + 56 * n);
		hash_to_point(logn, nonce, hashed_vk,
			ctx, ctx_len, id, hv, hv_len, hm);

		/* Initialize a sampler state. */
		sampler_state ss;
		sampler_init(&ss, logn, subseed, 56);

		/* Compute the lattice basis B = [[g, -f], [G, -F]] in FFT
		   representation, then compute the Gram matrix G = B*adj(B):
		      g00 = b00*adj(b00) + b01*adj(b01)
		      g01 = b00*adj(b10) + b01*adj(b11)
		      g10 = b10*adj(b00) + b11*adj(b01)
		      g11 = b10*adj(b10) + b11*adj(b11)
		   Note that g10 = adj(g01). For historical reasons, this
		   implementation keeps g01, i.e. the "upper triangle",
		   omitting g10.
		   We want the following layout:
		      free space (2n)
		      g00 (n)
		      g01 (n)
		      g11 (n)
		      b11 (n)
		      b01 (n)  */
		int8_t *f = (int8_t *)tmp;
		int8_t *g = f + n;
		(void)trim_i8_decode(logn, sign_key_fgF, f, nbits);
		(void)trim_i8_decode(logn, sign_key_fgF + flen, g, nbits);
		fpr *t0 = (fpr *)tmp;
		fpr *t1 = t0 + n;
		fpr *b00 = t1 + n;
		fpr *b01 = b00 + n;
		fpr *b10 = b01 + n;
		fpr *b11 = b10 + n;
		basis_to_FFT(logn, b00, b01, b10, b11, f, g, F, G);
		fpr *t2 = b11 + n;
		memcpy(t2, b01, n * sizeof(fpr));
		fpoly_gram_fft(logn, b00, b01, b10, b11);

		/* We now move things a bit to get the following (taking
		   into account that g00 and g11 are self-adjoint, hence
		   half-size):
		      free space (2n)
		      g01 (n)
		      g00 (hn)
		      g11 (hn)
		      free space (n)
		      b11 (n)
		      b01 (n)  */
		fpr *g01 = b00;
		fpr *g00 = b01;
		fpr *g11 = b01 + hn;
		memcpy(t1, b00, hn * sizeof(fpr));
		memcpy(g01, b01, n * sizeof(fpr));
		memcpy(g00, t1, hn * sizeof(fpr));
		memcpy(g11, b10, hn * sizeof(fpr));

		/* We now set the target [t0,t1] to [hm,0], then apply the
		   lattice basis to obtain the real target vector (after
		   normalization with regard to the modulus q).
		   b11 is unchanged, but b01 is in t2. */
		fpoly_apply_basis(logn, t0, t1, t2, b11, hm);

		/* Current layout:
		      t0  (n)
		      t1  (n)
		      g01 (n)
		      g00 (hn)
		      g11 (hn)
		   We now do the Fast Fourier sampling, which uses
		   up to 3*n slots beyond t1 (hence 7*n total usage
		   in tmp[]). */
		ffsamp_fft(&ss, tmp);

		/*
		 * At this point, [t0,t1] are the FFT representation of
		 * the sampled vector; in normal (non-FFT) representation,
		 * [t0,t1] is integral. We want to apply the lattice basis
		 * Compute the lattice basis B = [[g, -f], [G, -F]] to that
		 * vector, and subtract the result from [hm,0] to get the
		 * signature value. These computations can be done either
		 * in the FFT domain, or in with integers; and integer
		 * computations can be done modulo q = 12289 since the
		 * signature verification also works modulo q. Using
		 * integers is faster than staying in FFT representation
		 * when floating-point operations are emulated.
		 */
#if !(FNDSA_SSE2 || FNDSA_NEON || FNDSA_RV64D)

		/* Convert [t0, t1] to integers modulo q. */
		fpoly_iFFT(logn, t0);
		fpoly_iFFT(logn, t1);
		uint16_t *ut0 = (uint16_t *)(t1 + n);
		uint16_t *ut1 = ut0 + n;
		uint16_t *ut2 = ut1 + n;
		uint16_t *ut3 = ut2 + n;
#if FNDSA_SSE2
		/* We inline an fpr_rint() implementation, using SSE2
		   intrinsics (_mm_cvtpd_epi32() for rounding to neareast
		   with roundTiesToEven). */
		for (size_t i = 0; i < n; i += 2) {
			__m128d xt = _mm_loadu_pd((const double *)t0 + i);
			__m128i zt = _mm_cvtpd_epi32(xt);
			ut0[i + 0] = (uint16_t)_mm_cvtsi128_si32(zt);
			ut0[i + 1] = (uint16_t)_mm_cvtsi128_si32(
				_mm_bsrli_si128(zt, 4));
		}
		for (size_t i = 0; i < n; i += 2) {
			__m128d xt = _mm_loadu_pd((const double *)t1 + i);
			__m128i zt = _mm_cvtpd_epi32(xt);
			ut1[i + 0] = (uint16_t)_mm_cvtsi128_si32(zt);
			ut1[i + 1] = (uint16_t)_mm_cvtsi128_si32(
				_mm_bsrli_si128(zt, 4));
		}
#elif FNDSA_NEON
		/* We inline an fpr_rint() implementation, using NEON
		   intrinsics (vcvtnq_s64_f64() for rounding to neareast
		   with roundTiesToEven). */
		for (size_t i = 0; i < n; i += 2) {
			float64x2_t xt = vld1q_f64((const float64_t *)t0 + i);
			int64x2_t zt = vcvtnq_s64_f64(xt);
			ut0[i + 0] = (uint16_t)vgetq_lane_s64(zt, 0);
			ut0[i + 1] = (uint16_t)vgetq_lane_s64(zt, 1);
		}
		for (size_t i = 0; i < n; i += 2) {
			float64x2_t xt = vld1q_f64((const float64_t *)t1 + i);
			int64x2_t zt = vcvtnq_s64_f64(xt);
			ut1[i + 0] = (uint16_t)vgetq_lane_s64(zt, 0);
			ut1[i + 1] = (uint16_t)vgetq_lane_s64(zt, 1);
		}
#elif FNDSA_RV64D
		const f64 *tt0 = (const f64 *)t0;
		const f64 *tt1 = (const f64 *)t1;
		for (size_t i = 0; i < n; i ++) {
			ut0[i] = (uint64_t)f64_rint(tt0[i]);
		}
		for (size_t i = 0; i < n; i ++) {
			ut1[i] = (uint64_t)f64_rint(tt1[i]);
		}
#else
		for (size_t i = 0; i < n; i ++) {
			ut0[i] = (uint16_t)fpr_rint(t0[i]);
			ut1[i] = (uint16_t)fpr_rint(t1[i]);
		}
#endif
		mqpoly_signed_to_int(logn, ut0);
		mqpoly_signed_to_int(logn, ut1);

		/* Convert [t0,t1] to NTT. */
		mqpoly_int_to_ntt(logn, ut0);
		mqpoly_int_to_ntt(logn, ut1);

		/* Decode (f,g) from the signing key. */
		f = (int8_t *)(ut3 + n);
		g = f + n;
		(void)trim_i8_decode(logn, sign_key_fgF, f, nbits);
		(void)trim_i8_decode(logn, sign_key_fgF + flen, g, nbits);

		/* s1 = hm - (g*t0 + G*t1).
		   We compute s1 into ut3; we do not need to keep s1,
		   only its squared norm. */
		mqpoly_small_to_int(logn, g, ut2);
		mqpoly_small_to_int(logn, G, ut3);
		mqpoly_int_to_ntt(logn, ut2);
		mqpoly_int_to_ntt(logn, ut3);
		mqpoly_mul_ntt(logn, ut2, ut0);
		mqpoly_mul_ntt(logn, ut3, ut1);
		mqpoly_add(logn, ut2, ut3);
		mqpoly_ntt_to_int(logn, ut2);
		memcpy(ut3, hm, n * sizeof(uint16_t));
		mqpoly_ext_to_int(logn, ut3);
		mqpoly_sub(logn, ut3, ut2);
		uint32_t sqn1 = mqpoly_sqnorm_int(logn, ut3);

		/* s2 = -(-f*t0 - F*t1) = f*t0 + F*t1
		   We compute s2 into ut3. */
		mqpoly_small_to_int(logn, f, ut2);
		mqpoly_small_to_int(logn, F, ut3);
		mqpoly_int_to_ntt(logn, ut2);
		mqpoly_int_to_ntt(logn, ut3);
		mqpoly_mul_ntt(logn, ut2, ut0);
		mqpoly_mul_ntt(logn, ut3, ut1);
		mqpoly_add(logn, ut3, ut2);
		mqpoly_ntt_to_int(logn, ut3);
		uint32_t sqn2 = mqpoly_sqnorm_int_to_signed(logn, ut3);

		/* If either squared norm saturated, or the sum (i.e.
		   the total squared norm of [s1,s2]) is too high, then
		   we loop. */
		uint32_t sqn = sqn1 + sqn2;
		sqn1 |= sqn2;
		sqn |= (uint32_t)(*(int32_t *)&sqn1 >> 31);
		if (!mqpoly_sqnorm_is_acceptable(logn, sqn)) {
			continue;
		}
		int16_t *s2 = (int16_t *)ut3;

#else
		/*
		 * We stay here in the floating-point domain for the
		 * application of the basis. This happens to be faster
		 * on our test platforms with a hardware FPU.
		 */

		/* Get the lattice point corresponding to the sampled
		   vector. This means computing:
		      [v0,v1] = [t0,t1] * [[g, -f], [G, -F]]
		   hence:
		      v0 = t0*g + t1*G
		      v1 = -t0*f - t1*F  */
		fpr *w0 = t1 + n;
		fpr *w1 = w0 + n;
		f = (int8_t *)(w1 + n);
		g = f + n;
		(void)trim_i8_decode(logn, sign_key_fgF, f, nbits);
		(void)trim_i8_decode(logn, sign_key_fgF + flen, g, nbits);
		fpoly_set_small(logn, w0, g);
		fpoly_set_small(logn, w1, f);
		fpoly_FFT(logn, w0);
		fpoly_FFT(logn, w1);
		fpoly_mul_fft(logn, w1, t0);
		fpoly_mul_fft(logn, t0, w0);
		fpoly_set_small(logn, w0, G);
		fpoly_FFT(logn, w0);
		fpoly_mul_fft(logn, w0, t1);
		fpoly_add(logn, t0, w0);
		fpoly_set_small(logn, w0, F);
		fpoly_FFT(logn, w0);
		fpoly_mul_fft(logn, t1, w0);
		fpoly_add(logn, t1, w1);
		fpoly_neg(logn, t1);
		fpoly_iFFT(logn, t0);
		fpoly_iFFT(logn, t1);

		/* We compute s1, then s2 into buffer s2 (s1 is not
		   retained). We accumulate their squared norm in sqn,
		   with an "overflow" flag in ng. */
		uint32_t sqn = 0;
		uint32_t ng = 0;
		int16_t *s2 = (int16_t *)w0;
#if FNDSA_SSE2
		/* We inline an fpr_rint() implementation, using SSE2
		   intrinsics (_mm_cvtpd_epi32() for rounding to neareast
		   with roundTiesToEven). */
		for (size_t i = 0; i < n; i += 2) {
			__m128d xt = _mm_loadu_pd((const double *)t0 + i);
			__m128i zt = _mm_cvtpd_epi32(xt);
			uint16_t zu0 = hm[i + 0]
				- (uint16_t)_mm_cvtsi128_si32(zt);
			uint16_t zu1 = hm[i + 1]
				- (uint16_t)_mm_cvtsi128_si32(
					_mm_bsrli_si128(zt, 4));
			int32_t z0 = (int32_t)*(int16_t *)&zu0;
			int32_t z1 = (int32_t)*(int16_t *)&zu1;
			sqn += (uint32_t)(z0 * z0);
			ng |= sqn;
			sqn += (uint32_t)(z1 * z1);
			ng |= sqn;
		}
		for (size_t i = 0; i < n; i += 2) {
			__m128d xt = _mm_loadu_pd((const double *)t1 + i);
			__m128i zt = _mm_cvtpd_epi32(xt);
			uint16_t zu0 = -(uint16_t)_mm_cvtsi128_si32(zt);
			uint16_t zu1 = -(uint16_t)_mm_cvtsi128_si32(
				_mm_bsrli_si128(zt, 4));
			int32_t z0 = (int32_t)*(int16_t *)&zu0;
			int32_t z1 = (int32_t)*(int16_t *)&zu1;
			sqn += (uint32_t)(z0 * z0);
			ng |= sqn;
			sqn += (uint32_t)(z1 * z1);
			ng |= sqn;
			s2[i + 0] = (int16_t)z0;
			s2[i + 1] = (int16_t)z1;
		}
#elif FNDSA_NEON
		/* We inline an fpr_rint() implementation, using NEON
		   intrinsics (vcvtnq_s64_f64() for rounding to neareast
		   with roundTiesToEven). */
		for (size_t i = 0; i < n; i += 2) {
			float64x2_t xt = vld1q_f64((const float64_t *)t0 + i);
			int64x2_t zt = vcvtnq_s64_f64(xt);
			uint16_t zu0 = hm[i + 0]
				- (uint16_t)vgetq_lane_s64(zt, 0);
			uint16_t zu1 = hm[i + 1]
				- (uint16_t)vgetq_lane_s64(zt, 1);
			int32_t z0 = (int32_t)*(int16_t *)&zu0;
			int32_t z1 = (int32_t)*(int16_t *)&zu1;
			sqn += (uint32_t)(z0 * z0);
			ng |= sqn;
			sqn += (uint32_t)(z1 * z1);
			ng |= sqn;
		}
		for (size_t i = 0; i < n; i += 2) {
			float64x2_t xt = vld1q_f64((const float64_t *)t1 + i);
			int64x2_t zt = vcvtnq_s64_f64(xt);
			uint16_t zu0 = -(uint16_t)vgetq_lane_s64(zt, 0);
			uint16_t zu1 = -(uint16_t)vgetq_lane_s64(zt, 1);
			int32_t z0 = (int32_t)*(int16_t *)&zu0;
			int32_t z1 = (int32_t)*(int16_t *)&zu1;
			sqn += (uint32_t)(z0 * z0);
			ng |= sqn;
			sqn += (uint32_t)(z1 * z1);
			ng |= sqn;
			s2[i + 0] = (int16_t)z0;
			s2[i + 1] = (int16_t)z1;
		}
#elif FNDSA_RV64D
		const f64 *tt0 = (const f64 *)t0;
		const f64 *tt1 = (const f64 *)t1;
		for (size_t i = 0; i < n; i ++) {
			uint16_t zu = hm[i] - (uint16_t)f64_rint(tt0[i]);
			int32_t z = *(int16_t *)&zu;
			sqn += (uint32_t)(z * z);
			ng |= sqn;
		}
		for (size_t i = 0; i < n; i ++) {
			uint16_t zu = -(uint16_t)f64_rint(tt1[i]);
			int32_t z = *(int16_t *)&zu;
			sqn += (uint32_t)(z * z);
			ng |= sqn;
			s2[i] = (int16_t)z;
		}
#else
		for (size_t i = 0; i < n; i ++) {
			uint16_t zu = hm[i] - (uint16_t)fpr_rint(t0[i]);
			int32_t z = *(int16_t *)&zu;
			sqn += (uint32_t)(z * z);
			ng |= sqn;
		}
		for (size_t i = 0; i < n; i ++) {
			uint16_t zu = -(uint16_t)fpr_rint(t1[i]);
			int32_t z = *(int16_t *)&zu;
			sqn += (uint32_t)(z * z);
			ng |= sqn;
			s2[i] = (int16_t)z;
		}
#endif

		/* If the squared norm exceeds 2^31-1, then at some point
		   the high bit of ng was set, which we use to saturate
		   the squared norm to 2^32-1. If the squared norm is
		   unacceptable, then we loop. */
		sqn |= (uint32_t)(*(int32_t *)&ng >> 31);
		if (!mqpoly_sqnorm_is_acceptable(logn, sqn)) {
			continue;
		}
#endif

		/* We have a candidate signature; we must encode it. This
		   may fail, if the signature cannot be encoded in the
		   target size. */
		size_t sig_len = FNDSA_SIGNATURE_SIZE(logn);
		if (comp_encode(logn, s2, sig + 41, sig_len - 41)) {
			/* Success! */
			sig[0] = 0x30 + logn;
			memcpy(sig + 1, nonce, 40);
			ret = sig_len;
			goto sign_exit;
		}
	}

sign_exit:
#if FNDSA_SSE2
	_MM_SET_ROUNDING_MODE(round_mode);
#endif
	return ret;
}
