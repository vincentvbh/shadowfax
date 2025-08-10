/*
 * This file is not meant to be compiled independently, but to be
 * included (with #include) by another C file. The caller shall
 * first define the Q, N and LOGN macros to relevant values (decimal
 * literal constants only).
 */

#include "kem_params.h"

#if !defined Q || !defined N || !defined LOGN || !defined LVLBYTES
#error This module must not be compiled separately.
#endif

#include "kem_api.h"
#include "config.h"
#include "sys_rand.h"
#include "rng.h"
#include "encode_decode.h"
#include "kem257.h"
#include "keygen.h"

#include <string.h>

/* ====================================================================== */

/*
 * Get the length of sbuf, for a given degree n, with n = 2^logn.
 * The logn parameter must be between 1 and 10, inclusive. Returned length
 * is in bytes, between 1 and 128, inclusive.
 */
#define SBUF_LEN(logn)   (((1 << (logn)) + 7) >> 3)


/*
 * Macros for temporary buffer sizes.
 *
 * Each length is in bytes and accounts for an extra 31 bytes for internal
 * alignment adjustment.
 */
#define BAT_128_256_TMP_KEYGEN          6175
#define BAT_128_256_TMP_ENCAPS           799
#define BAT_128_256_TMP_DECAPS          2079

#define BAT_257_512_TMP_KEYGEN         12319
#define BAT_257_512_TMP_ENCAPS          2079
#define BAT_257_512_TMP_DECAPS          4127

#define BAT_769_1024_TMP_KEYGEN        24607
#define BAT_769_1024_TMP_ENCAPS         4127
#define BAT_769_1024_TMP_DECAPS         8223

/*
 * Error codes.
 */

/* Decapsulation failed. */
#define BAT_ERR_DECAPS_FAILED   -1

/* Provided object (key or ciphertext) uses a different set of parameters
   (modulus and/or degree) than expected by the called function. */
#define BAT_ERR_WRONG_PARAMS    -2

/* Provided object (key or ciphertext) is invalidly encoded. */
#define BAT_ERR_BAD_ENCODING    -3

/* Provided temporary space has insufficient length for the requested
   operation. */
#define BAT_ERR_NOSPACE         -4

/* Random seeding from operating system failed. */
#define BAT_ERR_RANDOM          -5

/*
 * Tag bytes. Each encoded public key, private key or ciphertext starts
 * with a tag byte that identifies the object type and parameters.
 * General format is (most-to-least significant order):
 *
 *    t t q q n n n n
 *
 * with:
 *
 *  - tt = 00 for a private key (long format), 01 for a private key (short
 *    format), 10 for a public key, 11 for a ciphertext.
 *  - qq = 00 for q = 128, 01 for q = 257, 10 for q = 769.
 *  - nnnn = log2(n) where n is the degree (power of 2, up to 1024).
 */
#define BAT_128_256_TAG_PRIVKEY_LONG     0x08
#define BAT_128_256_TAG_PRIVKEY_SHORT    0x48
#define BAT_128_256_TAG_PUBKEY           0x88
#define BAT_128_256_TAG_CIPHERTEXT       0xC8

#define BAT_257_512_TAG_PRIVKEY_LONG     0x19
#define BAT_257_512_TAG_PRIVKEY_SHORT    0x59
#define BAT_257_512_TAG_PUBKEY           0x99
#define BAT_257_512_TAG_CIPHERTEXT       0xD9

#define BAT_769_1024_TAG_PRIVKEY_LONG    0x2A
#define BAT_769_1024_TAG_PRIVKEY_SHORT   0x6A
#define BAT_769_1024_TAG_PUBKEY          0xAA
#define BAT_769_1024_TAG_CIPHERTEXT      0xEA

/*
 * Degrees up to 512 use BLAKE2s; degree 1024 uses BLAKE2b.
 */
// We use BLAKE2b.

// #define HASH           blake2s
// #define HASH_context   blake2s_context
// #define HASH_init      blake2s_init
// #define HASH_update    blake2s_update
// #define HASH_final     blake2s_final
// #define EXPAND         blake2s_expand

#define HASH           blake2b
#define HASH_context   blake2b_context
#define HASH_init      blake2b_init
#define HASH_update    blake2b_update
#define HASH_final     blake2b_final
#define EXPAND         blake2b_expand

/*
 * Recompute the additional secret seed (rr) from the private key seed.
 */
static void
make_rr(uint8_t *rr, uint8_t *seed){
	EXPAND(rr, SEED_BYTES, seed, SEED_BYTES, (uint32_t)Q | ((uint32_t)LOGN << 16) | 0x72000000);
}

/*
 * Compute the hash function Hash_m(), used over the plaintext polynomial
 * 's' to generate the encryption seed. Output size matches the security
 * level.
 */
static void
hash_m(void *dst, const void *sbuf, size_t sbuf_len){
	/*
	 * We use a raw hash here because in practice sbuf_len exactly
	 * matches the block length of the BLAKE2 function and we want
	 * to stick to a single invocation of the primitive.
	 *
	 * Note that the output size used here is at most 16 (with BLAKE2s,
	 * for degree N <= 512) or 32 (with BLAKE2b, for degree N = 1024),
	 * i.e. strictly less than the natural hash output size. The output
	 * size is part of the personalization block of BLAKE2, so this
	 * already ensures domain separation from the BLAKE2 invocations
	 * in the expand() calls in other functions used in this file.
	 */
	HASH(dst, LVLBYTES, NULL, 0, sbuf, sbuf_len);
}

/*
 * Compute the combination of Hash_s() and Sample_s(): the provided input
 * is nominally hashed into a seed, which is extended into enough bytes
 * with a KDF. The seed is used for nothing else. Moreover, the input is
 * guaranteed to be small (at most 32 bytes), so we can just use the
 * hash expand function.
 */
static void
hash_and_sample_s(void *sbuf, size_t sbuf_len, const void *m, size_t m_len){
	EXPAND(sbuf, sbuf_len, m, m_len, (uint32_t)Q | ((uint32_t)LOGN << 16) | 0x73000000);
}

/*
 * Make an alternate seed for key derivation, to be used on decapsulation
 * failure. This function is called F() in the BAT specification.
 */
static void
make_kdf_seed_bad(void *m, size_t m_len, const uint8_t *rr, const Zn(ct) *ct){
	HASH_context hc;
	uint8_t tmp[8];

	enc64le(tmp, (uint32_t)Q | ((uint32_t)LOGN << 16) | 0x62000000);
	HASH_init(&hc, m_len);
	HASH_update(&hc, tmp, sizeof tmp);
	HASH_update(&hc, rr, SEED_BYTES);
	HASH_update(&hc, ct->ct, N);
	HASH_final(&hc, m);
}

/*
 * Make the secret value from the plaintext s.
 * 'good' should be 1 for normal secret derivation, or 0 when doing
 * fake derivation after decapsulation failure.
 */
static void
make_secret(void *secret, size_t secret_len,
	const void *m, size_t m_len, uint32_t good)
{
	EXPAND(secret, secret_len, m, m_len,
		(uint32_t)Q | ((uint32_t)LOGN << 16) | ((good + 0x66) << 24));
}

/* see api.h */
static size_t
Zn(encode_h)(void *out, size_t max_out_len, const uint16_t *h){
	uint8_t *buf;
	size_t out_len, len;

	out_len = 1 + XCAT(bat_encode_, Q)(NULL, 0, h, LOGN);
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}
	buf = out;
	buf[0] = ZN(TAG_PUBKEY);
	len = XCAT(bat_encode_, Q)(buf + 1, max_out_len - 1, h, LOGN);
	if (len == 0) {
		return 0;
	}
	return 1 + len;
}

/* see api.h */
static size_t
Zn(decode_h)(uint16_t *h, const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	size_t len;

	if (max_in_len == 0) {
		return 0;
	}
	buf = in;
	if (buf[0] != ZN(TAG_PUBKEY)) {
		return 0;
	}
	len = XCAT(bat_decode_, Q)(h, LOGN, buf + 1, max_in_len - 1);
	if (len == 0) {
		return 0;
	}
	return 1 + len;
}

static size_t
get_privkey_length(int short_format)
{
	if (short_format) {
		return 1 + SEED_BYTES
			+ bat_trim_i8_encode(NULL, 0, NULL, LOGN, bat_max_FG_bits[LOGN]);
	} else {
		return 1 + 2 * SEED_BYTES
			+ bat_trim_i8_encode(NULL, 0,
				NULL, LOGN, bat_max_fg_bits[LOGN])
			+ bat_trim_i8_encode(NULL, 0,
				NULL, LOGN, bat_max_fg_bits[LOGN])
			+ bat_trim_i8_encode(NULL, 0,
				NULL, LOGN, bat_max_FG_bits[LOGN])
			+ bat_trim_i8_encode(NULL, 0,
				NULL, LOGN, bat_max_FG_bits[LOGN])
			+ bat_trim_i32_encode(NULL, 0,
				NULL, LOGN, bat_max_w_bits[LOGN])
			+ XCAT(bat_encode_, Q)(NULL, 0, NULL, LOGN);
	}
}

/* see api.h */
static size_t
Zn(encode_sk)(void *out, size_t max_out_len, const uint8_t *seed, const uint8_t *rr,
			   		   const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
                       const uint16_t *h, const int32_t *w,
			   		   int short_format)
{
	uint8_t *buf;
	size_t len, off, out_len;

	out_len = get_privkey_length(short_format);
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}
	buf = out;
	if (short_format) {
		buf[0] = ZN(TAG_PRIVKEY_SHORT);
		memmove(&buf[1], seed, SEED_BYTES);
		off = 1 + SEED_BYTES;
		len = bat_trim_i8_encode(buf + off, out_len - off, F, LOGN, bat_max_FG_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		return off;
	} else {
		buf[0] = ZN(TAG_PRIVKEY_LONG);
		memmove(&buf[1], seed, SEED_BYTES);
		off = 1 + SEED_BYTES;
		memmove(&buf[off], rr, SEED_BYTES);
		off += SEED_BYTES;
		len = bat_trim_i8_encode(buf + off, out_len - off, f, LOGN, bat_max_fg_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = bat_trim_i8_encode(buf + off, out_len - off, g, LOGN, bat_max_fg_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = bat_trim_i8_encode(buf + off, out_len - off, F, LOGN, bat_max_FG_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = bat_trim_i8_encode(buf + off, out_len - off, G, LOGN, bat_max_FG_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = bat_trim_i32_encode(buf + off, out_len - off, w, LOGN, bat_max_w_bits[LOGN]);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		len = XCAT(bat_encode_, Q)(buf + off, out_len - off, h, LOGN);
		if (len == 0) {
			/* This should never happen in practice. */
			return 0;
		}
		off += len;
		return off;
	}
}

/* see api.h */
static size_t
Zn(decode_sk)(uint8_t *seed, uint8_t *rr,
                       int8_t *f, int8_t *g, int8_t *F, int8_t *G,
                       uint16_t *h, int32_t *w,
                       const void *in, size_t max_in_len)
{

	const uint8_t *buf;
	size_t off, len;

	if (in == NULL || max_in_len == 0) {
		return 0;
	}
	buf = in;
	switch (buf[0]) {
	case ZN(TAG_PRIVKEY_SHORT):
		if (max_in_len < get_privkey_length(1)) {
			return 0;
		}
        __attribute__((aligned(8))) uint8_t tmp[9 * N];
		memmove(seed, buf + 1, SEED_BYTES);
		off = 1 + SEED_BYTES;
		len = bat_trim_i8_decode(F, LOGN, bat_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		if (!bat_keygen_make_fg(f, g, h, Q, LOGN, seed, SEED_BYTES, (uint32_t*)tmp))
		{
			return 0;
		}
		if (!bat_keygen_rebuild_G(G, f, g, F, Q, LOGN, (uint32_t*)tmp))
		{
			return 0;
		}
		if (!bat_keygen_compute_w(w, f, g, F, G, Q, LOGN, (uint32_t*)tmp))
		{
			return 0;
		}
		make_rr(rr, seed);
		return off;
	case ZN(TAG_PRIVKEY_LONG):
		if (max_in_len < get_privkey_length(0)) {
			return 0;
		}
		memmove(seed, buf + 1, SEED_BYTES);
		off = 1 + SEED_BYTES;
		memmove(rr, buf + off, SEED_BYTES);
		off += SEED_BYTES;
		len = bat_trim_i8_decode(f, LOGN, bat_max_fg_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = bat_trim_i8_decode(g, LOGN, bat_max_fg_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = bat_trim_i8_decode(F, LOGN, bat_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = bat_trim_i8_decode(G, LOGN, bat_max_FG_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = bat_trim_i32_decode(w, LOGN, bat_max_w_bits[LOGN],
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		len = XCAT(bat_decode_, Q)(h, LOGN,
			buf + off, max_in_len - off);
		if (len == 0) {
			return 0;
		}
		off += len;
		return off;
	default:
		return 0;
	}
}

/* see api.h */
int
Zn(keygen)(Zn(sk) *sk, Zn(pk) *pk)
{
    __attribute__((aligned(8))) uint8_t tmp[ZN(TMP_KEYGEN)];
    int8_t f[N];
    int8_t g[N];
    int8_t F[N];
    int8_t G[N];
    int32_t w[N];
    uint16_t h[N];
    uint8_t rr[SEED_BYTES];
    uint8_t seed[SEED_BYTES];
    uint8_t rng_seed[32];
	prng rng;

	if (!get_seed(rng_seed, sizeof rng_seed)) {
		return BAT_ERR_RANDOM;
	}
	prng_init(&rng, rng_seed, sizeof rng_seed, 0);
	for (;;) {
		prng_get_bytes(&rng, seed, SEED_BYTES);
		if (!bat_keygen_make_fg(f, g, h, Q, LOGN, seed, SEED_BYTES, (uint32_t*)tmp))
		{
			continue;
		}
		if (!bat_keygen_solve_FG(F, G, f, g, Q, LOGN, (uint32_t*)tmp))
		{
			continue;
		}
		if (!bat_keygen_compute_w(w, f, g, F, G, Q, LOGN, (uint32_t*)tmp))
		{
			continue;
		}
		make_rr(rr, seed);

		Zn(encode_h)(pk->pk, KEM_PUBLICKEY_BYTES, h);

        Zn(encode_sk)(sk->sk, KEM_SECRETKEY_BYTES,
                seed, rr,
                f, g, F, G,
                h, w, 0);

		return 0;
	}
}

/* see api.h */
size_t
Zn(encode_ct)(void *out, size_t max_out_len, const int8_t *c, const uint8_t *c2){
	uint8_t *buf;
	size_t out_len, len, off;

	out_len = 1 + XCAT(bat_encode_ct_, Q)(NULL, 0, c, LOGN) + C2_BYTES;
	if (out == NULL) {
		return out_len;
	}
	if (max_out_len < out_len) {
		return 0;
	}
	buf = out;
	buf[0] = ZN(TAG_CIPHERTEXT);
	off = 1;
	len = XCAT(bat_encode_ct_, Q)(buf + off, max_out_len - off, c, LOGN);
	if (len == 0) {
		return 0;
	}
	off += len;
	memcpy(buf + off, c2, C2_BYTES);
	off += C2_BYTES;
	return off;
}

/* see api.h */
size_t
Zn(decode_ct)(int8_t *c, uint8_t *c2, const void *in, size_t max_in_len)
{
	const uint8_t *buf;
	size_t off, len;

	if (max_in_len < 1) {
		return 0;
	}
	buf = in;
	if (buf[0] != ZN(TAG_CIPHERTEXT)) {
		return 0;
	}
	off = 1;
	len = XCAT(bat_decode_ct_, Q)(c, LOGN, buf + off, max_in_len - off);
	if (len == 0) {
		return 0;
	}
	off += len;
	if (max_in_len - off < C2_BYTES) {
		return 0;
	}
	memcpy(c2, buf + off, C2_BYTES);
	off += C2_BYTES;
	return off;
}

/* see api.h */
int
Zn(encap)(void *secret, size_t secret_len,
	Zn(ct) *ct, const Zn(pk) *pk)
{
	__attribute__((aligned(8))) uint8_t tmp[ZN(TMP_ENCAPS)];
	uint16_t h[KEM_PUBLICKEY_BYTES];
	int8_t c[N];
	uint8_t c2[C2_BYTES];

	Zn(decode_h)(h, pk->pk, KEM_PUBLICKEY_BYTES);

	/*
	 * Encapsulation may theoretically fail if the resulting
	 * vector norm is higher than a specific bound. However, this
	 * is very rare (it cannot happen at all for q = 257). Thus,
	 * we expect not to have to loop. Correspondingly, it is more
	 * efficient to use the random seed from the OS directly.
	 */
	for (;;) {
		uint8_t m[LVLBYTES], sbuf[SBUF_LEN(LOGN)];
		size_t u;

		/*
		 * Get a random message m from the OS.
		 */
		if (!get_seed(m, sizeof m)) {
			return BAT_ERR_RANDOM;
		}

		/*
		 * Hash m to sample s.
		 */
		hash_and_sample_s(sbuf, sizeof sbuf, m, sizeof m);
#if N < 8
		/* For very reduced toy versions, we don't even have a
		   full byte, and we must clear the unused bits. */
		sbuf[0] &= (1u << N) - 1u;
#endif

		/*
		 * Compute c1. This may fail (rarely!) only for q = 769.
		 */
		if (!XCAT(bat_encrypt_, Q)(c, sbuf, h, LOGN, (uint32_t*)tmp)) {
			continue;
		}

		/*
		 * Make c2 = Hash_m(s) XOR m.
		 */
		hash_m(c2, sbuf, sizeof sbuf);
		for (u = 0; u < sizeof m; u ++) {
			c2[u] ^= m[u];
		}

		Zn(encode_ct)(ct->ct, KEM_CIPHERTXT_BYTES, c, c2);

		/*
		 * Produce the shared secret (output of a successful key
		 * exchange).
		 */
		make_secret(secret, secret_len, m, sizeof m, 1);

		return 0;
	}
}

/* see api.h */
int
Zn(encap_seed)(void *secret, size_t secret_len,
	Zn(ct) *ct, const Zn(pk) *pk,
	const void *m)
{
    __attribute__((aligned(8))) uint8_t tmp[ZN(TMP_ENCAPS)];
	uint8_t m2[LVLBYTES];
	uint16_t h[KEM_PUBLICKEY_BYTES];
	int8_t c[N];
	uint8_t c2[C2_BYTES];

	Zn(decode_h)(h, pk->pk, KEM_PUBLICKEY_BYTES);

	for (;;) {
		uint8_t sbuf[SBUF_LEN(LOGN)];
		size_t u;

		/*
		 * If no seed is provided, then generate one randomly.
		 */
		if (m == NULL) {
			if (!get_seed(m2, sizeof m2)) {
				return BAT_ERR_RANDOM;
			}
			m = m2;
		}

		/*
		 * Hash m to sample s.
		 */
		hash_and_sample_s(sbuf, sizeof sbuf, m, LVLBYTES);
#if N < 8
		/* For very reduced toy versions, we don't even have a
		   full byte, and we must clear the unused bits. */
		sbuf[0] &= (1u << N) - 1u;
#endif

		/*
		 * Compute c1. This may fail (very rarely!) only for q = 769;
		 * we just hash the current seed. Since this occurrence is
		 * very rare in practice, this process does not induce any
		 * non-negligible bias.
		 */
		if (!XCAT(bat_encrypt_, Q)(c, sbuf, h, LOGN, (uint32_t*)tmp)) {
			blake2s(m2, LVLBYTES, NULL, 0, m, LVLBYTES);
			m = m2;
			continue;
		}

		/*
		 * Make c2 = Hash_m(s) XOR m.
		 */
		hash_m(c2, sbuf, sizeof sbuf);
		for (u = 0; u < LVLBYTES; u ++) {
			c2[u] ^= ((const uint8_t *)m)[u];
		}

		Zn(encode_ct)(ct->ct, KEM_CIPHERTXT_BYTES, c, c2);

		/*
		 * Produce the shared secret (output of a successful key
		 * exchange).
		 */
		make_secret(secret, secret_len, m, LVLBYTES, 1);

		return 0;
	}
}

/* see api.h */
int
Zn(decap)(void *secret, size_t secret_len,
	const Zn(ct) *ct, const Zn(sk) *sk)
{
    __attribute__((aligned(8))) uint8_t tmp[ZN(TMP_DECAPS)];
    int8_t f[N];
    int8_t g[N];
    int8_t F[N];
    int8_t G[N];
    int32_t w[N];
    uint16_t h[N];
    uint8_t rr[SEED_BYTES];
    uint8_t seed[SEED_BYTES];
	uint8_t sbuf[SBUF_LEN(LOGN)], m[LVLBYTES], m_alt[LVLBYTES];
	uint8_t sbuf_alt[SBUF_LEN(LOGN)];
	int8_t c[N];
	uint8_t c2[C2_BYTES];
	int8_t *c_alt;
	size_t u;
	uint32_t d;

    Zn(decode_sk)(
            seed, rr,
            f, g, F, G,
            h, w,
            sk->sk, KEM_SECRETKEY_BYTES);

	Zn(decode_ct)(c, c2, ct->ct, KEM_CIPHERTXT_BYTES);

	/*
	 * Inner decryption never fails (at least, it never reports
	 * a failure).
	 */
	XCAT(bat_decrypt_, Q)(sbuf, c, f, g, F, G, w, LOGN, (uint32_t*)tmp);

	/*
	 * From sbuf, we derive the mask that allows recovery of m
	 * out of the second ciphertext half (c2).
	 */
	hash_m(m, sbuf, sizeof sbuf);
	for (u = 0; u < sizeof m; u ++) {
		m[u] ^= c2[u];
	}

	/*
	 * Decryption is valid if and only if we can reencrypt the
	 * obtained message m and get the exact same polynomial s
	 * and ciphertext c1.
	 */
	hash_and_sample_s(sbuf_alt, sizeof sbuf_alt, m, sizeof m);
#if N < 8
	sbuf_alt[0] &= (1u << N) - 1u;
#endif
	c_alt = (int8_t*)tmp;
	d = XCAT(bat_encrypt_, Q)(c_alt, sbuf_alt, h, LOGN, (uint32_t*)tmp);
	d --;
	for (u = 0; u < sizeof sbuf; u ++) {
		d |= sbuf[u] ^ sbuf_alt[u];
	}
	for (u = 0; u < N; u ++) {
		d |= (uint32_t)(c[u] - c_alt[u]);
	}

	/*
	 * If encapsulation worked AND yielded the same ciphertext as
	 * received, then d == 0 at this point, and we want to produce
	 * the secret key as a hash of m. Otherwise, d != 0, and we
	 * must produce the secret as a hash of the received ciphertext
	 * and the secret value r (stored in sk->rr). We MUST NOT leak
	 * which was the case, and therefore we must always compute
	 * both hashes and perform constant-time conditional replacement.
	 */

	make_kdf_seed_bad(m_alt, sizeof m, rr, ct);
	d = -((uint32_t)(d | -d) >> 31);
	for (u = 0; u < sizeof m; u ++) {
		m[u] ^= d & (m[u] ^ m_alt[u]);
	}
	make_secret(secret, secret_len, m, sizeof m, d + 1);
	return 0;
}

