#ifndef KEM257_H
#define KEM257_H

#include <stddef.h>
#include <stdint.h>

/*
 * Compute the public key h = g/f. Returned value is 1 on success, 0 on
 * error. An error is reported if f is not invertible modulo X^n+1.
 * This function is for q = 257 and 1 <= logn <= 9.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int bat_make_public_257(uint16_t *h, const int8_t *f, const int8_t *g,
    unsigned logn, uint32_t *tmp);

/*
 * Given f, g and F, rebuild G, for the case q = 257. This function
 * reports a failure if (q,logn) are not supported parameters, if f is
 * not invertible modulo x^n+1 and modulo q, or if the rebuilt value G
 * has coefficients that exceed the expected maximum size.
 *
 * This function does NOT check that the returned G matches the NTRU
 * equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int bat_rebuild_G_257(int8_t *G,
    const int8_t *f, const int8_t *g, const int8_t *F,
    unsigned logn, uint32_t *tmp);

/*
 * Encrypt: given public key (in h) and secret polynomial s (in sbuf[]),
 * produce ciphertext c1 (in c).
 *
 * This function is for q = 257, with logn = 1 to 9. Ciphertext elements
 * are in the -64..+64 range. The function cannot fail, hence it always
 * returns 1.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
uint32_t bat_encrypt_257(int8_t *c, const uint8_t *sbuf,
    const uint16_t *h, unsigned logn, uint32_t *tmp);

/*
 * Decrypt: given private key (f,g,F,G,w) and ciphertext c1, extract
 * secret s. The polynomial s has length n bits (with n = 2^logn); it
 * is returned in sbuf[] (ceil(n/8) bytes; for toy versions with logn <
 * 3, the upper bits of the incomplete byte are set to zero).
 *
 * This function is for q = 257. Ciphertext elements are in the -64..+64
 * range.
 *
 * Size of tmp[]: 2*n elements (8*n bytes).
 *
 * This function never fails; for proper security, the caller must obtain
 * the message m (using the second ciphertext element c2) and check that
 * encryption of m would indeed yield exactly ciphertext c1.
 */
void bat_decrypt_257(uint8_t *sbuf, const int8_t *c,
    const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
    const int32_t *w, unsigned logn, uint32_t *tmp);

/*
 * Second phase of decapsulation, performed modulo 257.
 * Given c', c'', f, F and w, this function computes:
 *    Fd = q'*F - f*w
 *    q*q'*Q*s' = Fd*c' - f*c''
 *
 * On input, cp[] and cs[] must contain c' and c'', respectively, in
 * Montgomery representation modulo 257. On output, polynomial q*q'*Q*s'
 * is returned in cp[], in Montgomery representation modulo 257 (since
 * coefficients of s' can have only a few specific values, this is enough
 * to recover s'). cs[] is consumed. tmp[] must have room for 4*n bytes
 * (n 32-bit elements).
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
void bat_finish_decapsulate_257(uint16_t *cp, uint16_t *cs,
    const int8_t *f, const int8_t *F, const int32_t *w, unsigned logn,
    uint32_t *tmp);

/*
 * Encode a polynomial with coefficients modulo 257. This is used for
 * public keys with q = 257.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t bat_encode_257(void *out, size_t max_out_len,
    const uint16_t *x, unsigned logn);

/*
 * Decode a polynomial with coefficients modulo 257. This is used for
 * public keys with q = 257.
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
size_t bat_decode_257(uint16_t *x, unsigned logn,
    const void *in, size_t max_in_len);

/*
 * Encode a ciphertext polynomial, for q = 257; coefficients are in -64..+64.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t bat_encode_ct_257(void *out, size_t max_out_len,
    const int8_t *c, unsigned logn);
/*
 * Decode a ciphertext polynomial, for q = 257; coefficients are in -64..+64.
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
size_t bat_decode_ct_257(int8_t *c, unsigned logn,
    const void *in, size_t max_in_len);

#endif

