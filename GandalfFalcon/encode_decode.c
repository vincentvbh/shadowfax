
#include "encode_decode.h"
#include "rsig_params.h"

#include "inner.h"

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

    return comp_encode(LOG_N, src16, des, COMPRESSED_SIGN_SIGNATURE_BYTES);
    // return comp_encode(des, COMPRESSED_SIGN_SIGNATURE_BYTES, src16, LOG_N);

}

size_t decompress_u_to_poly(int32_t *des, const void *src){

    int16_t des16[N];
    size_t bytes;

    bytes = comp_decode(LOG_N, src, COMPRESSED_SIGN_SIGNATURE_BYTES, des16);
    // bytes = comp_decode(des16, LOG_N, src, COMPRESSED_SIGN_SIGNATURE_BYTES);

    for(size_t i = 0; i < N; i++){
        des[i] = des16[i];
    }

    return bytes;

}








