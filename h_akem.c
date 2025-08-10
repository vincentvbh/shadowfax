
#include "h_akem_api.h"
#include "aes.h"
#include "hmac.h"
#include "fips202.h"

#include <string.h>

static const uint8_t aes_iv[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void h_akem_keygen_expanded_sk(h_akem_expanded_sk *sk, h_akem_pk *pk){
    kem_keygen(&sk->ksk, &pk->kpk);
    sign_keygen_expanded_sk(&sk->ssk, &pk->spk);
    nike_keygen(&sk->nsk, &pk->npk);
}

void h_akem_keygen(h_akem_sk *sk, h_akem_pk *pk){
    kem_keygen(&sk->ksk, &pk->kpk);
    sign_keygen(&sk->ssk, &pk->spk);
    nike_keygen(&sk->nsk, &pk->npk);
}

void h_akem_encap_expanded_sk(uint8_t *h_akem_k, h_akem_ct *ct,
                              const h_akem_expanded_sk *sender_expanded_sk, const h_akem_pk *sender_pk,
                              const h_akem_pk *receiver_pk){

    kem_ct internal_kem_ct;
    rsig_pk internal_rsig_pk;
    rsig_signature internal_signature;
    nike_sk e_nsk;
    nike_pk e_npk;
    nike_s nk, nkprime, nk1k2;
    uint8_t k1k2[64];
    uint8_t hmac_nk2[32];
    uint8_t hmac_out[32];
    uint8_t kprime[32];
    uint8_t m[MLEN];
    uint8_t enc_rsig[RSIG_SIGNATURE_BYTES];
    const uint8_t tag[4] = "auth";
    aes128ctx ctx;
    sha3_256incctx hmac_state;

    uint8_t *k1 = k1k2;
    uint8_t *k2 = k1 + 32;
    uint8_t *nk1 = nk1k2.s;
    uint8_t *nk2 = nk1 + 32;

    nike_keygen(&e_nsk, &e_npk);
    nike_sdk(&nkprime, &sender_expanded_sk->nsk, &receiver_pk->npk);
    hmac_sha3_256((uint8_t*)&nk.s, tag, sizeof(tag), nkprime.s);
    nike_sdk(&nk1k2, &e_nsk, &receiver_pk->npk);

    kem_encap(k1k2, 64, &internal_kem_ct, &receiver_pk->kpk);

    memmove(m, &internal_kem_ct, KEM_CIPHERTXT_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES, &receiver_pk->kpk, KEM_PUBLICKEY_BYTES);

    internal_rsig_pk.hs[0] = sender_pk->spk;
    internal_rsig_pk.hs[1] = receiver_pk->spk;

    Gandalf_sign_expanded_sk(&internal_signature, m, MLEN, &internal_rsig_pk, &sender_expanded_sk->ssk, 0);

    hmac_sha3_256(kprime, k1, 32, nk1);

    aes128_ctr_keyexp(&ctx, kprime);
    aes128_ctr(enc_rsig, (void*)&internal_signature, RSIG_SIGNATURE_BYTES, aes_iv, &ctx);
    aes128_ctx_release(&ctx);

    ct->npk = e_npk;
    ct->ct = internal_kem_ct;
    memmove(ct->enc_rsig, enc_rsig, RSIG_SIGNATURE_BYTES);

    hmac_sha3_256_inc_init(&hmac_state, k2);
    sha3_256_inc_absorb(&hmac_state, (const uint8_t*)ct, sizeof(h_akem_ct));
    sha3_256_inc_absorb(&hmac_state, (const uint8_t*)sender_pk, sizeof(h_akem_pk));
    sha3_256_inc_absorb(&hmac_state, (const uint8_t*)receiver_pk, sizeof(h_akem_pk));
    hmac_sha3_256_inc_finalize(hmac_out, &hmac_state, k2);
    sha3_256_inc_ctx_release(&hmac_state);

    hmac_sha3_256(hmac_nk2, nk2, 32, nk.s);
    hmac_sha3_256(h_akem_k, hmac_out, 32, hmac_nk2);

}

void h_akem_encap(uint8_t *h_akem_k, h_akem_ct *ct,
                  const h_akem_sk *sender_sk, const h_akem_pk *sender_pk, const h_akem_pk *receiver_pk){

    h_akem_expanded_sk sender_expanded_sk;

    sender_expanded_sk.ksk = sender_sk->ksk;
    expand_sign_sk(&sender_expanded_sk.ssk, &sender_sk->ssk);
    sender_expanded_sk.nsk = sender_sk->nsk;
    h_akem_encap_expanded_sk(h_akem_k, ct, &sender_expanded_sk, sender_pk, receiver_pk);

}

int h_akem_decap(uint8_t *h_akem_k, const h_akem_ct *ct,
                 const h_akem_sk *receiver_sk, const h_akem_pk *receiver_pk,
                 const h_akem_pk *sender_pk){

    rsig_pk internal_rsig_pk;
    nike_s nk, nkprime, nk1k2;
    uint8_t k1k2[64];
    uint8_t hmac_nk2[32];
    uint8_t hmac_out[32];
    uint8_t kprime[32];
    uint8_t m[MLEN];
    uint8_t dec_rsig[RSIG_SIGNATURE_BYTES];
    const uint8_t tag[4] = "auth";
    aes128ctx ctx;
    sha3_256incctx hmac_state;

    uint8_t *k1 = k1k2;
    uint8_t *k2 = k1 + 32;
    uint8_t *nk1 = nk1k2.s;
    uint8_t *nk2 = nk1 + 32;

    nike_sdk(&nkprime, &receiver_sk->nsk, &sender_pk->npk);
    hmac_sha3_256((uint8_t*)&nk.s, tag, sizeof(tag), nkprime.s);
    nike_sdk(&nk1k2, &receiver_sk->nsk, &ct->npk);

    kem_decap(k1k2, 64, &ct->ct, &receiver_sk->ksk);

    hmac_sha3_256(kprime, k1, 32, nk1);

    memmove(m, &ct->ct, KEM_CIPHERTXT_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES, &receiver_pk->kpk, KEM_PUBLICKEY_BYTES);

    aes128_ctr_keyexp(&ctx, kprime);
    aes128_ctr(dec_rsig, ct->enc_rsig, RSIG_SIGNATURE_BYTES, aes_iv, &ctx);
    aes128_ctx_release(&ctx);

    internal_rsig_pk.hs[0] = sender_pk->spk;
    internal_rsig_pk.hs[1] = receiver_pk->spk;

    if(Gandalf_verify(m, MLEN, (const rsig_signature*)dec_rsig, &internal_rsig_pk) == 0){
        return 0;
    }

    hmac_sha3_256_inc_init(&hmac_state, k2);
    sha3_256_inc_absorb(&hmac_state, (const uint8_t*)ct, sizeof(h_akem_ct));
    sha3_256_inc_absorb(&hmac_state, (const uint8_t*)sender_pk, sizeof(h_akem_pk));
    sha3_256_inc_absorb(&hmac_state, (const uint8_t*)receiver_pk, sizeof(h_akem_pk));
    hmac_sha3_256_inc_finalize(hmac_out, &hmac_state, k2);
    sha3_256_inc_ctx_release(&hmac_state);

    hmac_sha3_256(hmac_nk2, nk2, 32, nk.s);
    hmac_sha3_256(h_akem_k, hmac_out, 32, hmac_nk2);

    return 1;

}

