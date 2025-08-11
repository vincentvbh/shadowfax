#ifndef ENCODE_DECODE_H
#define ENCODE_DECODE_H

#include "poly.h"

#include <stdint.h>
#include <stddef.h>

size_t comp_encode(void *out, size_t max_out_len, const int16_t *x, unsigned logn);
size_t comp_decode(int16_t *x, unsigned logn, const void *in, size_t max_in_len);
size_t modq_encode(void *out, size_t max_out_len, const uint16_t *x, unsigned logn);
size_t modq_decode(uint16_t *x, unsigned logn, const void *in, size_t max_in_len);
size_t modq_encode32(void *out, size_t max_out_len, const uint32_t *x, unsigned logn);
size_t modq_decode32(uint32_t *x, unsigned logn, const void *in, size_t max_in_len);
size_t compress_u_from_poly(void *des, const int32_t *src);
size_t decompress_u_to_poly(int32_t *des, const void *src);

#endif

