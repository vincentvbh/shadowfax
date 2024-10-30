#ifndef H_AKEM_API_H
#define H_AKEM_API_H

#include "nike_api.h"
#include "kem_api.h"
#include "rsig_api.h"

#define H_AKEM_SECRETKEY_BYTES 5033
#define H_AKEM_PUBLICKEY_BYTES 1449
#define H_AKEM_CIPHERTXT_BYTES 1781
#define H_AKEM_CRYPTO_BYTES 32
#define MLEN (KEM_CIPHERTXT_BYTES + KEM_PUBLICKEY_BYTES)

typedef struct {
    nike_sk nsk;
    kem_sk ksk;
    sign_sk ssk;
} h_akem_sk;

typedef struct {
    nike_sk nsk;
    kem_sk ksk;
    sign_expanded_sk ssk;
} h_akem_expanded_sk;

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


void h_akem_keygen_expanded_sk(h_akem_expanded_sk *sk, h_akem_pk *pk);
void h_akem_keygen(h_akem_sk *sk, h_akem_pk *pk);

void h_akem_encap_expanded_sk(uint8_t *h_akem_k, h_akem_ct *ct,
                              const h_akem_expanded_sk *sender_expanded_sk, const h_akem_pk *sender_pk,
                              const h_akem_pk *receiver_pk);
void h_akem_encap(uint8_t *h_akem_k, h_akem_ct *ct,
                  const h_akem_sk *sender_sk, const h_akem_pk *sender_pk, const h_akem_pk *receiver_pk);

int h_akem_decap(uint8_t *h_akem_k, const h_akem_ct *ct,
                 const h_akem_sk *receiver_sk, const h_akem_pk *receiver_pk,
                 const h_akem_pk *sender_pk);

#endif

