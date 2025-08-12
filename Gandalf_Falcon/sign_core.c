/*
 * Core signature generation.
 */

#include "sign_inner.h"

#include <stdio.h>

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

void
trapdoor_sampler(unsigned logn,
		// fpr *t0, fpr *t1,
		int16_t *s1, int16_t *s2,
		const int8_t *f_src, const int8_t *g_src, const int8_t *F, const int8_t *G,
		const uint16_t *c, const uint8_t *subseed,
		void *tmp) {

		size_t n = (size_t)1 << logn;
		size_t hn = n >> 1;

		sampler_state ss;
		sampler_init(&ss, logn, subseed, 56);

		int8_t f[n];
		int8_t g[n];
		memcpy(f, f_src, n * sizeof(int8_t));
		memcpy(g, g_src, n * sizeof(int8_t));
		fpr t0[n];
		fpr t1[n];
		fpr b00[n];
		fpr b01[n];
		fpr b10[n];
		fpr b11[n];
		basis_to_FFT(logn, b00, b01, b10, b11, f, g, F, G);
		fpr b01_cache[n];
		memcpy(b01_cache, b01, n * sizeof(fpr));
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
		fpr g01[n];
		fpr g00[hn];
		fpr g11[hn];
		memcpy(t1, b00, hn * sizeof(fpr));
		memcpy(g01, b01, n * sizeof(fpr));
		memcpy(g00, t1, hn * sizeof(fpr));
		memcpy(g11, b10, hn * sizeof(fpr));

		/* We now set the target [t0,t1] to [hm,0], then apply the
		   lattice basis to obtain the real target vector (after
		   normalization with regard to the modulus q).
		   b11 is unchanged, but b01 is in b01_cache. */
		fpoly_apply_basis(logn, t0, t1, b01_cache, b11, c);

		/* Current layout:
		      t0  (n)
		      t1  (n)
		      g01 (n)
		      g00 (hn)
		      g11 (hn)
		   We now do the Fast Fourier sampling, which uses
		   up to 3*n slots beyond t1 (hence 7*n total usage
		   in tmp[]). */
		fpr *tmp_fpr = (fpr*)tmp;
		memcpy(tmp_fpr, t0, n * sizeof(fpr));
		memcpy(tmp_fpr + n, t1, n * sizeof(fpr));
		memcpy(tmp_fpr + 2 * n, g01, n * sizeof(fpr));
		memcpy(tmp_fpr + 3 * n, g00, hn * sizeof(fpr));
		memcpy(tmp_fpr + 3 * n + hn, g11, hn * sizeof(fpr));
		ffsamp_fft(&ss, tmp);
		memcpy(t0, tmp_fpr, n * sizeof(fpr));
		memcpy(t1, tmp_fpr + n, n * sizeof(fpr));

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
		fpr w0[n];
		fpr w1[n];
		memcpy(f, f_src, n * sizeof(int8_t));
		memcpy(g, g_src, n * sizeof(int8_t));
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

		for(size_t i = 0; i < n; i++){
			s1[i] = (int16_t)(c[i] - (uint16_t)fpr_rint(t0[i]));
		}

		for(size_t i = 0; i < n; i++){
			s2[i] = (int16_t)(-(uint16_t)fpr_rint(t1[i]));
		}

}

static
uint32_t compute_sqn(const size_t n, int16_t *s1, int16_t *s2){
		/* We compute the saturated squared norm in sqn with
		   an "overflow" flag in ng. */
		uint32_t sqn = 0;
		uint32_t ng = 0;
		int32_t z;
		for(size_t i = 0; i < n; i++){
			z = (int32_t)s1[i];
			sqn += (uint32_t)(z * z);
			ng |= sqn;
		}
		for(size_t i = 0; i < n; i++){
			z = (int32_t)s2[i];
			sqn += (uint32_t)(z * z);
			ng |= sqn;
		}
		/* If the squared norm exceeds 2^31 - 1, we saturate it to
		   2^32 - 1. */
		sqn |= (uint32_t)(*(int32_t *)&ng >> 31);
		return sqn;
}

int check_norm(const size_t logn, int16_t *s1, int16_t *s2) {
		size_t n = (size_t)1 << logn;
		uint32_t sqn = compute_sqn(n, s1, s2);
		return mqpoly_sqnorm_is_acceptable(logn, sqn);
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
	uint8_t nonce[40];
	uint8_t subseed[56];

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
			shake_context sc;
			shake_init(&sc, 256);
			shake_inject(&sc, seed, seed_len);
			uint8_t cbuf[4];
			cbuf[0] = (uint8_t)counter;
			cbuf[1] = (uint8_t)(counter >> 8);
			cbuf[2] = (uint8_t)(counter >> 16);
			cbuf[3] = (uint8_t)(counter >> 24);
			shake_inject(&sc, cbuf, 4);
			shake_flip(&sc);
			shake_extract(&sc, rndp, rndlen);
		}

		memcpy(nonce, rndbuf, 40 * sizeof(uint8_t));
		memcpy(subseed, rndbuf + 40, 56 * sizeof(uint8_t));

		/* Hash the message into a polynomial. */
		uint16_t hm[n];
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
		int8_t f_src[n];
		int8_t g_src[n];
		(void)trim_i8_decode(logn, sign_key_fgF, f_src, nbits);
		(void)trim_i8_decode(logn, sign_key_fgF + flen, g_src, nbits);

		fpr t0[n];
		fpr t1[n];

		int16_t s1[n];
		int16_t s2[n];

		trapdoor_sampler(logn, s1, s2, f_src, g_src, F, G, hm, subseed, tmp);

		/* If the squared norm is unacceptable, then we loop. */
		if(!check_norm(logn, s1, s2)) {
			continue;
		}

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
