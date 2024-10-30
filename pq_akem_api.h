#ifndef PQ_AKEM_API_H
#define PQ_AKEM_API_H

#include "kem_api.h"
#include "rsig_api.h"

#define PQ_AKEM_SECRETKEY_BYTES 5001
#define PQ_AKEM_PUBLICKEY_BYTES 1417
#define PQ_AKEM_CIPHERTXT_BYTES 1749
#define PQ_AKEM_CRYPTO_BYTES 32
#define MLEN (KEM_CIPHERTXT_BYTES + 2 * KEM_PUBLICKEY_BYTES + SIGN_PUBLICKEY_BYTES)

typedef struct {
    kem_sk ksk;
    sign_sk ssk;
} pq_akem_sk;

typedef struct {
    kem_sk ksk;
    sign_expanded_sk ssk;
} pq_akem_expanded_sk;

typedef struct {
    kem_pk kpk;
    sign_pk spk;
} pq_akem_pk;

typedef struct {
    kem_ct ct;
    uint8_t enc_rsig[RSIG_SIGNATURE_BYTES];
} pq_akem_ct;

void pq_akem_keygen_expanded_sk(pq_akem_expanded_sk *expanded_sk, pq_akem_pk *pk);
void pq_akem_keygen(pq_akem_sk *sk, pq_akem_pk *pk);

void pq_akem_encap_expanded_sk(uint8_t *pq_akem_k, pq_akem_ct *ct,
                const pq_akem_expanded_sk *sender_expanded_sk, const pq_akem_pk *sender_pk, const pq_akem_pk *receiver_pk);
void pq_akem_encap(uint8_t *pq_akem_k, pq_akem_ct *ct,
                const pq_akem_sk *sender_sk, const pq_akem_pk *sender_pk, const pq_akem_pk *receiver_pk);

int pq_akem_decap(uint8_t *pq_akem_k, const pq_akem_ct *ct,
               const pq_akem_sk *receiver_sk, const pq_akem_pk *receiver_pk, const pq_akem_pk *sender_pk);

#endif

