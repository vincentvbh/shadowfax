#ifndef H_AKEM_API_H
#define H_AKEM_API_H

#include "nike_api.h"
#include "kem_api.h"
#include "rsig_api.h"

typedef struct {
    nike_sk nsk;
    kem_sk ksk;
    sign_sk ssk;
} h_akem_sk;

typedef struct {
    nike_pk npk;
    kem_pk kpk;
    sign_pk spk;
} h_akem_pk;

typedef struct {
    nike_pk npk;
    kem_ct ct;
    uint8_t enc_rsig[RSIG_SIGNATURE_BYTES];
} h_akem_ct;

#define H_AKEM_SECRETKEY_BYTES sizeof(h_akem_sk)
#define H_AKEM_PUBLICKEY_BYTES sizeof(h_akem_pk)
#define H_AKEM_CIPHERTXT_BYTES sizeof(h_akem_ct)
#define H_AKEM_CRYPTO_BYTES 32
// Message length for the ring signature.
#define MLEN (KEM_CIPHERTXT_BYTES + KEM_PUBLICKEY_BYTES)

void h_akem_keygen(h_akem_sk *sk, h_akem_pk *pk);

void h_akem_encap(uint8_t *h_akem_k, h_akem_ct *ct,
                  const h_akem_sk *sender_sk, const h_akem_pk *sender_pk, const h_akem_pk *receiver_pk);

int h_akem_decap(uint8_t *h_akem_k, const h_akem_ct *ct,
                 const h_akem_sk *receiver_sk, const h_akem_pk *receiver_pk,
                 const h_akem_pk *sender_pk);

#endif

