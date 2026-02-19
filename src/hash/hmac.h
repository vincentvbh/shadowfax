#ifndef HMAC_H
#define HMAC_H

#include "fips202.h"

#include <stdint.h>
#include <stddef.h>

#define HMAC_KEYBYTES 32
#define HMAC_SHA3_256_BYTES 32

void hmac_sha3_256_inc_init(sha3_256incctx *ctx, const uint8_t *k);
void hmac_sha3_256_inc_finalize(uint8_t *out, sha3_256incctx *ctx, const uint8_t *k);
int hmac_sha3_256(uint8_t *out, const uint8_t *in, size_t inlen, const uint8_t *k);

#endif

