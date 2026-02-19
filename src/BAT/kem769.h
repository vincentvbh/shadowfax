#ifndef KEM769_H
#define KEM769_H

#include <stddef.h>
#include <stdint.h>

/*
 * Compute the public key h = g/f. Returned value is 1 on success, 0 on
 * error. An error is reported if f is not invertible modulo X^n+1.
 * This function is for q = 769 and 1 <= logn <= 10.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int bat_make_public_769(uint16_t *h, const int8_t *f, const int8_t *g,
    unsigned logn, uint32_t *tmp);

/*
 * Given f, g and F, rebuild G, for the case q = 769. This function
 * reports a failure if (q,logn) are not supported parameters, if f is
 * not invertible modulo x^n+1 and modulo q, or if the rebuilt value G
 * has coefficients that exceed the expected maximum size.
 *
 * This function does NOT check that the returned G matches the NTRU
 * equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int bat_rebuild_G_769(int8_t *G,
    const int8_t *f, const int8_t *g, const int8_t *F,
    unsigned logn, uint32_t *tmp);

/*
 * Encrypt: given public key (in h) and secret polynomial s (in sbuf[]),
 * produce ciphertext c1 (in c).
 *
 * This function is for q = 769, with logn = 1 to 10. Ciphertext elements
 * are in the -96..+96 range.
 *
 * The function may fail, if the norm of the result is too high, in which
 * case the caller should start again with a new seed (this is uncommon).
 * On failure, this function returns 0; on success, it returns 1.
 *
 * Size of tmp[]: 3*n/4 elements (3*n bytes).
 */
uint32_t bat_encrypt_769(int8_t *c, const uint8_t *sbuf,
    const uint16_t *h, unsigned logn, uint32_t *tmp);

/*
 * Decrypt: given private key (f,g,F,G,w) and ciphertext c1, extract
 * secret s. The polynomial s has length n bits (with n = 2^logn); it
 * is returned in sbuf[] (ceil(n/8) bytes; for toy versions with logn <
 * 3, the upper bits of the incomplete byte are set to zero).
 *
 * This function is for q = 769. Ciphertext elements are in the -96..+96
 * range.
 *
 * Size of tmp[]: 2*n elements (8*n bytes).
 *
 * This function never fails; for proper security, the caller must obtain
 * the message m (using the second ciphertext element c2) and check that
 * encryption of m would indeed yield exactly ciphertext c1.
 */
void bat_decrypt_769(uint8_t *sbuf, const int8_t *c,
    const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
    const int32_t *w, unsigned logn, uint32_t *tmp);

/*
 * Second phase of decapsulation, performed modulo 769.
 * Given c', c'', f, F and w, this function computes:
 *    Fd = q'*F - f*w
 *    q*q'*Q*s' = Fd*c' - f*c''
 *
 * On input, cp[] and cs[] must contain c' and c'', respectively, in
 * Montgomery representation modulo 769. On output, polynomial q*q'*Q*s'
 * is returned in cp[], in Montgomery representation modulo 769 (since
 * coefficients of s' can have only a few specific values, this is enough
 * to recover s'). cs[] is consumed. tmp[] must have room for 4*n bytes
 * (n 32-bit elements).
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
void bat_finish_decapsulate_769(uint16_t *cp, uint16_t *cs,
    const int8_t *f, const int8_t *F, const int32_t *w, unsigned logn,
    uint32_t *tmp);

/*
 * Encode a polynomial with coefficients modulo 769. This is used for
 * public keys with q = 769.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t bat_encode_769(void *out, size_t max_out_len,
    const uint16_t *x, unsigned logn);

/*
 * Decode a polynomial with coefficients modulo 769. This is used for
 * public keys with q = 769.
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
size_t bat_decode_769(uint16_t *x, unsigned logn,
    const void *in, size_t max_in_len);

/*
 * Encode a ciphertext polynomial, for q = 769; coefficients are in -96..+96.
 *
 * If out == NULL, then max_out_len is ignored and the function returns
 * the size of the output it could produce (in bytes).
 * If out != NULL, then max_out_len is compared with the expected output
 * size. If max_out_len is lower, then no output is produced, and the
 * function returns 0; otherwise, the output is produced and its length
 * (in bytes) is returned.
 */
size_t bat_encode_ct_769(void *out, size_t max_out_len,
    const int8_t *c, unsigned logn);
/*
 * Decode a ciphertext polynomial, for q = 769; coefficients are in -96..+96.
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
size_t bat_decode_ct_769(int8_t *c, unsigned logn,
    const void *in, size_t max_in_len);

#endif

