#ifndef ENCODE_DECODE_H
#define ENCODE_DECODE_H

#include <stdint.h>
#include <stddef.h>

/* ====================================================================== */

/*
 * Max size in bits for elements of (f,g), indexed by log(N). Size includes
 * the sign bit.
 */
extern const uint8_t bat_max_fg_bits[];

/*
 * Max size in bits for elements of (F,G), indexed by log(N). Size includes
 * the sign bit.
 */
extern const uint8_t bat_max_FG_bits[];

/*
 * Max size in bits for elements of w, indexed by log(N). Size includes
 * the sign bit.
 */
extern const uint8_t bat_max_w_bits[];

/* ====================================================================== */
/*
 * Encoding/decoding functions.
 */

static inline unsigned
dec16le(const void *src)
{
    const uint8_t *buf;

    buf = src;
    return (unsigned)buf[0]
        | ((unsigned)buf[1] << 8);
}

static inline void
enc16le(void *dst, unsigned x)
{
    uint8_t *buf;

    buf = dst;
    buf[0] = (uint8_t)x;
    buf[1] = (uint8_t)(x >> 8);
}

static inline uint32_t
dec32le(const void *src)
{
    const uint8_t *buf;

    buf = src;
    return (uint32_t)buf[0]
        | ((uint32_t)buf[1] << 8)
        | ((uint32_t)buf[2] << 16)
        | ((uint32_t)buf[3] << 24);
}

static inline void
enc32le(void *dst, uint32_t x)
{
    uint8_t *buf;

    buf = dst;
    buf[0] = (uint8_t)x;
    buf[1] = (uint8_t)(x >> 8);
    buf[2] = (uint8_t)(x >> 16);
    buf[3] = (uint8_t)(x >> 24);
}

static inline uint64_t
dec64le(const void *src)
{
    const uint8_t *buf;

    buf = src;
    return (uint64_t)buf[0]
        | ((uint64_t)buf[1] << 8)
        | ((uint64_t)buf[2] << 16)
        | ((uint64_t)buf[3] << 24)
        | ((uint64_t)buf[4] << 32)
        | ((uint64_t)buf[5] << 40)
        | ((uint64_t)buf[6] << 48)
        | ((uint64_t)buf[7] << 56);
}

static inline void
enc64le(void *dst, uint64_t x)
{
    uint8_t *buf;

    buf = dst;
    buf[0] = (uint64_t)x;
    buf[1] = (uint64_t)(x >> 8);
    buf[2] = (uint64_t)(x >> 16);
    buf[3] = (uint64_t)(x >> 24);
    buf[4] = (uint64_t)(x >> 32);
    buf[5] = (uint64_t)(x >> 40);
    buf[6] = (uint64_t)(x >> 48);
    buf[7] = (uint64_t)(x >> 56);
}

static inline uint32_t
dec24le(const void *src)
{
    const uint8_t *buf;

    buf = src;
    return (uint32_t)buf[0]
        | ((uint32_t)buf[1] << 8)
        | ((uint32_t)buf[2] << 16);
}

static inline void
enc24le(void *dst, uint32_t x)
{
    uint8_t *buf;

    buf = dst;
    buf[0] = (uint8_t)x;
    buf[1] = (uint8_t)(x >> 8);
    buf[2] = (uint8_t)(x >> 16);
}

/* ====================================================================== */

/*
 * bat_trim_i32_encode() and bat_trim_i32_decode() encode and decode
 * polynomials with signed coefficients (int32_t), using the specified
 * number of bits for each coefficient. The number of bits includes the
 * sign bit. Each coefficient x must be such that |x| < 2^(bits-1) (the
 * value -2^(bits-1), though conceptually encodable with two's
 * complement representation, is forbidden).
 *
 * bat_trim_i8_encode() and bat_trim_i8_decode() do the same work for
 * polynomials whose coefficients are held in slots of type int8_t.
 *
 * Encoding API:
 *
 *   Output buffer (out[]) has max length max_out_len (in bytes). If
 *   that length is not large enough, then no encoding occurs and the
 *   function returns 0; otherwise, the function returns the number of
 *   bytes which have been written into out[]. If out == NULL, then
 *   max_out_len is ignored, and no output is produced, but the function
 *   returns how many bytes it would produce.
 *
 *   Encoding functions assume that the input is valid (all values in
 *   the encodable range).
 *
 * Decoding API:
 *
 *   Input buffer (in[]) has maximum length max_in_len (in bytes). If
 *   the input length is not enough for the expected polynomial, then
 *   no decoding occurs and the function returns 0. Otherwise, the values
 *   are decoded and the number of processed input bytes is returned.
 *
 *   If the input is invalid in some way (a decoded coefficient has
 *   value -2^(bits-1), or some of the ignored bits in the last byte
 *   are non-zero), then the function fails and returns 0; the contents
 *   of the output array are then indeterminate.
 *
 * Both encoding and decoding are constant-time with regards to the
 * values and bits.
 */

size_t bat_trim_i32_encode(void *out, size_t max_out_len,
    const int32_t *x, unsigned logn, unsigned bits);
size_t bat_trim_i32_decode(int32_t *x, unsigned logn, unsigned bits,
    const void *in, size_t max_in_len);
size_t bat_trim_i8_encode(void *out, size_t max_out_len,
    const int8_t *x, unsigned logn, unsigned bits);
size_t bat_trim_i8_decode(int8_t *x, unsigned logn, unsigned bits,
    const void *in, size_t max_in_len);

#endif
