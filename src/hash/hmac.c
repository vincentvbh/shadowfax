/*
 * 20080913
 * D. J. Bernstein
 * Public domain.
 * */

#include "hmac.h"
#include "fips202.h"

#include <stdint.h>

#define blocks crypto_hashblocks

void hmac_sha3_256_inc_init(sha3_256incctx *ctx, const uint8_t *k){

    uint8_t padded[32];

    for(size_t i = 0; i < 32; i++){
        padded[i] = k[i] ^ 0x36;
    }

    sha3_256_inc_init(ctx);
    sha3_256_inc_absorb(ctx, padded, 32);

}

void hmac_sha3_256_inc_finalize(uint8_t *out, sha3_256incctx *ctx, const uint8_t *k){

    uint8_t h[32];
    uint8_t padded[32];

    sha3_256_inc_finalize(h, ctx);
    sha3_256_inc_ctx_release(ctx);

    for(size_t i = 0; i < 32; i++){
        padded[i] = k[i] ^ 0x5c;
    }

    sha3_256_inc_init(ctx);
    sha3_256_inc_absorb(ctx, padded, 32);
    sha3_256_inc_absorb(ctx, h, 32);
    sha3_256_inc_finalize(out, ctx);

    sha3_256_inc_ctx_release(ctx);

}

int hmac_sha3_256(uint8_t *out, const uint8_t *in, size_t inlen, const uint8_t *k){

    sha3_256incctx ctx;

    hmac_sha3_256_inc_init(&ctx, k);
    sha3_256_inc_absorb(&ctx, in, inlen);
    hmac_sha3_256_inc_finalize(out, &ctx, k);

    return 0;

}






