#ifndef KEM128_H
#define KEM128_H

#include <stddef.h>
#include <stdint.h>

/*
 * Compute the public key h = g/f. Returned value is 1 on success, 0 on
 * error. An error is reported if f is not invertible modulo X^n+1.
 * This function is for q = 128 and 1 <= logn <= 8.
 * CAUTION: for q = 128, public key is in an array of uint8_t, not uint16_t.
 *
 * Size of tmp[]: 3*n/4 elements (3*n bytes).
 */
int bat_make_public_128(uint8_t *h, const int8_t *f, const int8_t *g,
    unsigned logn, uint32_t *tmp);

/*
 * Given f, g and F, rebuild G, for the case q = 128. This function
 * reports a failure if (q,logn) are not supported parameters, if f is
 * not invertible modulo x^n+1 and modulo q, or if the rebuilt value G
 * has coefficients that exceed the expected maximum size.
 *
 * This function does NOT check that the returned G matches the NTRU
 * equation.
 *
 * Size of tmp[]: 3*n/4 elements (3*n bytes).
 */
int bat_rebuild_G_128(int8_t *G,
    const int8_t *f, const int8_t *g, const int8_t *F,
    unsigned logn, uint32_t *tmp);

/*
 * Encrypt: given public key (in h) and secret polynomial s (in sbuf[]),
 * produce ciphertext c1 (in c).
 *
 * This function is for q = 128, with logn = 1 to 8. Ciphertext elements
 * are in the -31..+32 range. The function cannot fail, hence it always
 * returns 1.
 * CAUTION: for q = 128, public key is in an array of uint8_t, not uint16_t.
 *
 * Size of tmp[]: 3*n/4 elements (3*n bytes)
 */
uint32_t bat_encrypt_128(int8_t *c, const uint8_t *sbuf,
    const uint8_t *h, unsigned logn, uint32_t *tmp);

/*
 * Decrypt: given private key (f,g,F,G,w) and ciphertext c1, extract
 * secret s. The polynomial s has length n bits (with n = 2^logn); it
 * is returned in sbuf[] (ceil(n/8) bytes; for toy versions with logn <
 * 3, the upper bits of the incomplete byte are set to zero).
 *
 * This function is for q = 128. Ciphertext elements are in the -31..+32
 * range.
 *
 * Size of tmp[]: 2*n elements (8*n bytes).
 *
 * This function never fails; for proper security, the caller must obtain
 * the message m (using the second ciphertext element c2) and check that
 * encryption of m would indeed yield exactly ciphertext c1.
 */
void bat_decrypt_128(uint8_t *sbuf, const int8_t *c,
    const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
    const int32_t *w, unsigned logn, uint32_t *tmp);

/*
 * Encode a polynomial with coefficients modulo 128. This is used for
 * public keys with q = 128.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t bat_encode_128(void *out, size_t max_out_len,
    const uint8_t *x, unsigned logn);

/*
 * Decode a polynomial with coefficients modulo 128. This is used for
 * public keys with q = 128.
 *
 * Input buffer (in[]) has maximum length max_in_len (in bytes). If
 * the input length is not enough for the expected polynomial, then
 * no decoding occurs and the function returns 0. Otherwise, the values
 * are decoded and the number of processed input bytes is returned.
 *
 * If the input is invalid in some way (a decoded coefficient is out of
 * the expected range, or some ignored bit is non-zero), then this the
 * function fails and returns 0; the contents of the output array are
 * then indeterminate.
 *
 * Decoding is constant-time as long as no failure occurs.
 */
size_t bat_decode_128(uint8_t *x, unsigned logn,
    const void *in, size_t max_in_len);

/*
 * Encode a ciphertext polynomial, for q = 128; coefficients are in -31..+32.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t bat_encode_ct_128(void *out, size_t max_out_len,
    const int8_t *c, unsigned logn);
/*
 * Decode a ciphertext polynomial, for q = 128; coefficients are in -31..+32.
 *
 * Input buffer (in[]) has maximum length max_in_len (in bytes). If
 * the input length is not enough for the expected polynomial, then
 * no decoding occurs and the function returns 0. Otherwise, the values
 * are decoded and the number of processed input bytes is returned.
 *
 * If the input is invalid in some way (a decoded coefficient is out of
 * the expected range, or some ignored bit is non-zero), then this the
 * function fails and returns 0; the contents of the output array are
 * then indeterminate.
 *
 * Decoding is constant-time with regard to the coefficient values.
 */
size_t bat_decode_ct_128(int8_t *c, unsigned logn,
    const void *in, size_t max_in_len);

#endif

