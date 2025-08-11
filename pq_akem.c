
#include "pq_akem_api.h"
#include "aes.h"
#include "fips202.h"

#include <string.h>

static const uint8_t aes_iv[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void pq_akem_keygen_expanded_sk(pq_akem_expanded_sk *expanded_sk, pq_akem_pk *pk){
    kem_keygen(&expanded_sk->ksk, &pk->kpk);
    sign_keygen_expanded_sk(&expanded_sk->ssk, &pk->spk);
}

void pq_akem_keygen(pq_akem_sk *sk, pq_akem_pk *pk){
    kem_keygen(&sk->ksk, &pk->kpk);
    sign_keygen(&sk->ssk, &pk->spk);
}

void pq_akem_encap_expanded_sk(uint8_t *pq_akem_k, pq_akem_ct *ct,
                               const pq_akem_expanded_sk *sender_expanded_sk, const pq_akem_pk *sender_pk,
                               const pq_akem_pk *receiver_pk){

    kem_ct internal_kem_ct;
    rsig_pk internal_rsig_pk;
    rsig_signature internal_signature;
    uint8_t kk[48];
    uint8_t m[MLEN];
    uint8_t enc_rsig[RSIG_SIGNATURE_BYTES];
    aes128ctx ctx;
    sha3_256incctx hash_state;

    kem_encap(kk, 48, &internal_kem_ct, &receiver_pk->kpk);

    memmove(m, &internal_kem_ct, KEM_CIPHERTXT_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES, &sender_pk->kpk, KEM_PUBLICKEY_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES + KEM_PUBLICKEY_BYTES, &receiver_pk->kpk, KEM_PUBLICKEY_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES + 2 * KEM_PUBLICKEY_BYTES, &receiver_pk->spk, SIGN_PUBLICKEY_BYTES);

    internal_rsig_pk.hs[0] = sender_pk->spk;
    internal_rsig_pk.hs[1] = receiver_pk->spk;

    Gandalf_sign_expanded_sk(&internal_signature, m, MLEN, &internal_rsig_pk, &sender_expanded_sk->ssk, 0);

    aes128_ctr_keyexp(&ctx, kk);
    aes128_ctr(enc_rsig, (void*)&internal_signature, RSIG_SIGNATURE_BYTES, aes_iv, &ctx);
    aes128_ctx_release(&ctx);

    ct->ct = internal_kem_ct;
    memmove(ct->enc_rsig, enc_rsig, RSIG_SIGNATURE_BYTES);

    sha3_256_inc_init(&hash_state);
    sha3_256_inc_absorb(&hash_state, kk + 16, 32);
    sha3_256_inc_absorb(&hash_state, enc_rsig, RSIG_SIGNATURE_BYTES);
    sha3_256_inc_absorb(&hash_state, (const uint8_t *)&sender_pk->spk, SIGN_PUBLICKEY_BYTES);
    sha3_256_inc_absorb(&hash_state, m, MLEN);
    sha3_256_inc_finalize(pq_akem_k, &hash_state);
    sha3_256_inc_ctx_release(&hash_state);

}

void pq_akem_encap(uint8_t *pq_akem_k, pq_akem_ct *ct,
                   const pq_akem_sk *sender_sk, const pq_akem_pk *sender_pk, const pq_akem_pk *receiver_pk){

    pq_akem_expanded_sk sender_expanded_sk;

    sender_expanded_sk.ksk = sender_sk->ksk;
    expand_sign_sk(&sender_expanded_sk.ssk, &sender_sk->ssk);
    pq_akem_encap_expanded_sk(pq_akem_k, ct, &sender_expanded_sk, sender_pk, receiver_pk);

}

int pq_akem_decap(uint8_t *pq_akem_k, const pq_akem_ct *ct,
                  const pq_akem_sk *receiver_sk, const pq_akem_pk *receiver_pk,
                  const pq_akem_pk *sender_pk){

    rsig_pk internal_rsig_pk;
    uint8_t kk[48];
    uint8_t m[MLEN];
    uint8_t dec_rsig[RSIG_SIGNATURE_BYTES];
    aes128ctx ctx;
    sha3_256incctx hash_state;

    kem_decap(kk, 48, &ct->ct, &receiver_sk->ksk);

    aes128_ctr_keyexp(&ctx, kk);
    aes128_ctr(dec_rsig, (void*)&ct->enc_rsig, RSIG_SIGNATURE_BYTES, aes_iv, &ctx);
    aes128_ctx_release(&ctx);

    memmove(m, &ct->ct, KEM_CIPHERTXT_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES, &sender_pk->kpk, KEM_PUBLICKEY_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES + KEM_PUBLICKEY_BYTES, &receiver_pk->kpk, KEM_PUBLICKEY_BYTES);
    memmove(m + KEM_CIPHERTXT_BYTES + 2 * KEM_PUBLICKEY_BYTES, &receiver_pk->spk, SIGN_PUBLICKEY_BYTES);

    internal_rsig_pk.hs[0] = sender_pk->spk;
    internal_rsig_pk.hs[1] = receiver_pk->spk;

    if(Gandalf_verify(m, MLEN, (const rsig_signature*)dec_rsig, &internal_rsig_pk) == 0){
        return 0;
    }

    sha3_256_inc_init(&hash_state);
    sha3_256_inc_absorb(&hash_state, kk + 16, 32);
    sha3_256_inc_absorb(&hash_state, ct->enc_rsig, RSIG_SIGNATURE_BYTES);
    sha3_256_inc_absorb(&hash_state, (const uint8_t *)&sender_pk->spk, SIGN_PUBLICKEY_BYTES);
    sha3_256_inc_absorb(&hash_state, m, MLEN);
    sha3_256_inc_finalize(pq_akem_k, &hash_state);
    sha3_256_inc_ctx_release(&hash_state);

    return 1;

}



