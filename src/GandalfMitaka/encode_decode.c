
#include "encode_decode.h"
#include "rsig_params.h"

/* see encode_decode.h */
size_t comp_encode(
    void *out, size_t max_out_len,
    const int16_t *x, unsigned logn)
{
    uint8_t *buf;
    size_t n, u, v;
    uint32_t acc;
    unsigned acc_len;

    n = (size_t)1 << logn;
    buf = out;

    /*
     * Make sure that all values are within the -2047..+2047 range.
     */
    for (u = 0; u < n; u ++) {
        if (x[u] < -2047 || x[u] > +2047) {
            return 0;
        }
    }

    acc = 0;
    acc_len = 0;
    v = 0;
    for (u = 0; u < n; u ++) {
        int t;
        unsigned w;

        /*
         * Get sign and absolute value of next integer; push the
         * sign bit.
         */
        acc <<= 1;
        t = x[u];
        if (t < 0) {
            t = -t;
            acc |= 1;
        }
        w = (unsigned)t;

        /*
         * Push the low 7 bits of the absolute value.
         */
        acc <<= 7;
        acc |= w & 127u;
        w >>= 7;

        /*
         * We pushed exactly 8 bits.
         */
        acc_len += 8;

        /*
         * Push as many zeros as necessary, then a one. Since the
         * absolute value is at most 2047, w can only range up to
         * 15 at this point, thus we will add at most 16 bits
         * here. With the 8 bits above and possibly up to 7 bits
         * from previous iterations, we may go up to 31 bits, which
         * will fit in the accumulator, which is an uint32_t.
         */
        acc <<= (w + 1);
        acc |= 1;
        acc_len += w + 1;

        /*
         * Produce all full bytes.
         */
        while (acc_len >= 8) {
            acc_len -= 8;
            if (buf != NULL) {
                if (v >= max_out_len) {
                    return 0;
                }
                buf[v] = (uint8_t)(acc >> acc_len);
            }
            v ++;
        }
    }

    /*
     * Flush remaining bits (if any).
     */
    if (acc_len > 0) {
        if (buf != NULL) {
            if (v >= max_out_len) {
                return 0;
            }
            buf[v] = (uint8_t)(acc << (8 - acc_len));
        }
        v ++;
    }

    return v;
}

/* see encode_decode.h */
size_t comp_decode(
    int16_t *x, unsigned logn,
    const void *in, size_t max_in_len)
{
    const uint8_t *buf;
    size_t n, u, v;
    uint32_t acc;
    unsigned acc_len;

    n = (size_t)1 << logn;
    buf = in;
    acc = 0;
    acc_len = 0;
    v = 0;
    for (u = 0; u < n; u ++) {
        unsigned b, s, m;

        /*
         * Get next eight bits: sign and low seven bits of the
         * absolute value.
         */
        if (v >= max_in_len) {
            return 0;
        }
        acc = (acc << 8) | (uint32_t)buf[v ++];
        b = acc >> acc_len;
        s = b & 128;
        m = b & 127;

        /*
         * Get next bits until a 1 is reached.
         */
        for (;;) {
            if (acc_len == 0) {
                if (v >= max_in_len) {
                    return 0;
                }
                acc = (acc << 8) | (uint32_t)buf[v ++];
                acc_len = 8;
            }
            acc_len --;
            if (((acc >> acc_len) & 1) != 0) {
                break;
            }
            m += 128;
            if (m > 2047) {
                return 0;
            }
        }

        /*
         * "-0" is forbidden.
         */
        if (s && m == 0) {
            return 0;
        }

        x[u] = (int16_t)(s ? -(int)m : (int)m);
    }

    /*
     * Unused bits in the last byte must be zero.
     */
    if ((acc & ((1u << acc_len) - 1u)) != 0) {
        return 0;
    }

    return v;
}

/* see encode_decode.h */
size_t modq_encode(void *out, size_t max_out_len, const uint16_t *x, unsigned logn){

    size_t n, out_len, u;
    uint8_t *buf;
    uint32_t acc;
    int acc_len;

    n = (size_t)1 << logn;
    for (u = 0; u < n; u ++) {
        if (x[u] >= 12289) {
            return 0;
        }
    }
    out_len = ((n * 14) + 7) >> 3;
    if (out == NULL) {
        return out_len;
    }
    if (out_len > max_out_len) {
        return 0;
    }
    buf = out;
    acc = 0;
    acc_len = 0;
    for (u = 0; u < n; u ++) {
        acc = (acc << 14) | x[u];
        acc_len += 14;
        while (acc_len >= 8) {
            acc_len -= 8;
            *buf ++ = (uint8_t)(acc >> acc_len);
        }
    }
    if (acc_len > 0) {
        *buf = (uint8_t)(acc << (8 - acc_len));
    }
    return out_len;
}

/* see encode_decode.h */
size_t modq_decode(uint16_t *x, unsigned logn, const void *in, size_t max_in_len){

    size_t n, in_len, u;
    const uint8_t *buf;
    uint32_t acc;
    int acc_len;

    n = (size_t)1 << logn;
    in_len = ((n * 14) + 7) >> 3;
    if (in_len > max_in_len) {
        return 0;
    }
    buf = in;
    acc = 0;
    acc_len = 0;
    u = 0;
    while (u < n) {
        acc = (acc << 8) | (*buf ++);
        acc_len += 8;
        if (acc_len >= 14) {
            unsigned w;

            acc_len -= 14;
            w = (acc >> acc_len) & 0x3FFF;
            if (w >= 12289) {
                return 0;
            }
            x[u ++] = (uint16_t)w;
        }
    }
    if ((acc & (((uint32_t)1 << acc_len) - 1)) != 0) {
        return 0;
    }
    return in_len;
}

/* see encode_decode.h */
size_t modq_encode32(void *out, size_t max_out_len, const uint32_t *x, unsigned logn){

    size_t n, out_len, u;
    uint8_t *buf;
    uint32_t acc;
    int acc_len;

    n = (size_t)1 << logn;
    for (u = 0; u < n; u ++) {
        if (x[u] >= 12289) {
            return 0;
        }
    }
    out_len = ((n * 14) + 7) >> 3;
    if (out == NULL) {
        return out_len;
    }
    if (out_len > max_out_len) {
        return 0;
    }
    buf = out;
    acc = 0;
    acc_len = 0;
    for (u = 0; u < n; u ++) {
        acc = (acc << 14) | x[u];
        acc_len += 14;
        while (acc_len >= 8) {
            acc_len -= 8;
            *buf ++ = (uint8_t)(acc >> acc_len);
        }
    }
    if (acc_len > 0) {
        *buf = (uint8_t)(acc << (8 - acc_len));
    }
    return out_len;
}

/* see encode_decode.h */
size_t modq_decode32(uint32_t *x, unsigned logn, const void *in, size_t max_in_len){

    size_t n, in_len, u;
    const uint8_t *buf;
    uint32_t acc;
    int acc_len;

    n = (size_t)1 << logn;
    in_len = ((n * 14) + 7) >> 3;
    if (in_len > max_in_len) {
        return 0;
    }
    buf = in;
    acc = 0;
    acc_len = 0;
    u = 0;
    while (u < n) {
        acc = (acc << 8) | (*buf ++);
        acc_len += 8;
        if (acc_len >= 14) {
            unsigned w;

            acc_len -= 14;
            w = (acc >> acc_len) & 0x3FFF;
            if (w >= 12289) {
                return 0;
            }
            x[u ++] = w;
        }
    }
    if ((acc & (((uint32_t)1 << acc_len) - 1)) != 0) {
        return 0;
    }
    return in_len;
}

size_t compress_u_from_poly(void *des, const int32_t *src){

    int16_t src16[N];

    for(size_t i = 0; i < N; i++){
        src16[i] = src[i];
    }

    return comp_encode(des, COMPRESSED_SIGN_SIGNATURE_BYTES, src16, LOG_N);

}

size_t decompress_u_to_poly(int32_t *des, const void *src){

    int16_t des16[N];
    size_t bytes;

    bytes = comp_decode(des16, LOG_N, src, COMPRESSED_SIGN_SIGNATURE_BYTES);

    for(size_t i = 0; i < N; i++){
        des[i] = des16[i];
    }

    return bytes;

}








