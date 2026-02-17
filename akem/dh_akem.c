
#include "dh_akem_api.h"
#include "curve25519.h"
#include "scalarmult.h"
#include "fips202.h"

void nike_akem_keygen(nike_sk *sk, nike_pk *pk){
    dh_keypair(sk->sk, pk->pk);
}

static
const uint8_t dh_prefix[] = {"HPKE-v1curve25519eae_prk"};

static
const uint8_t context_prefix[] = {"32HPKE-v1curve25519shared_secret"};

void nike_akem_encap(uint8_t *k, nike_pk *ct,
            const nike_sk *sender_sk, const nike_pk *sender_pk, const nike_pk *receiver_pk){

    nike_sk e_nsk;
    nike_pk e_npk;
    uint8_t k1[32], k2[32];
    sha3_256incctx ctx;

    nike_keygen(&e_nsk, &e_npk);

    scalarmult(k1, sender_sk->sk, receiver_pk->pk);
    scalarmult(k2, e_nsk.sk, receiver_pk->pk);

    *ct = e_npk;

    sha3_256_inc_init(&ctx);
    sha3_256_inc_absorb(&ctx, dh_prefix, sizeof(dh_prefix));
    sha3_256_inc_absorb(&ctx, k1, 32);
    sha3_256_inc_absorb(&ctx, k2, 32);
    sha3_256_inc_absorb(&ctx, context_prefix, sizeof(context_prefix));
    sha3_256_inc_absorb(&ctx, (const uint8_t*)&e_npk, sizeof(nike_pk));
    sha3_256_inc_absorb(&ctx, (const uint8_t*)receiver_pk, sizeof(nike_pk));
    sha3_256_inc_absorb(&ctx, (const uint8_t*)sender_pk, sizeof(nike_pk));
    sha3_256_inc_finalize(k, &ctx);
    sha3_256_inc_ctx_release(&ctx);

}

void nike_akem_decap(uint8_t *k,
            const nike_pk *ct, const nike_sk *receiver_sk, const nike_pk *receiver_pk, const nike_pk *sender_pk){

    uint8_t k1[32], k2[32];
    sha3_256incctx ctx;

    scalarmult(k1, receiver_sk->sk, sender_pk->pk);
    scalarmult(k2, receiver_sk->sk, ct->pk);

    sha3_256_inc_init(&ctx);
    sha3_256_inc_absorb(&ctx, dh_prefix, sizeof(dh_prefix));
    sha3_256_inc_absorb(&ctx, k1, 32);
    sha3_256_inc_absorb(&ctx, k2, 32);
    sha3_256_inc_absorb(&ctx, context_prefix, sizeof(context_prefix));
    sha3_256_inc_absorb(&ctx, (const uint8_t*)ct, sizeof(nike_pk));
    sha3_256_inc_absorb(&ctx, (const uint8_t*)receiver_pk, sizeof(nike_pk));
    sha3_256_inc_absorb(&ctx, (const uint8_t*)sender_pk, sizeof(nike_pk));
    sha3_256_inc_finalize(k, &ctx);
    sha3_256_inc_ctx_release(&ctx);

}



