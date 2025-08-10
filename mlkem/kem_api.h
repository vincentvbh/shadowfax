#ifndef KEM_API_H
#define KEM_APIH_

#include <stdint.h>
#include <stddef.h>

#include "params.h"

#define KEM_PUBLICKEY_BYTES KYBER_PUBLICKEYBYTES
#define KEM_CIPHERTXT_BYTES KYBER_CIPHERTEXTBYTES
#define KEM_SECRETKEY_BYTES KYBER_SECRETKEYBYTES

typedef struct {
    uint8_t sk[KEM_SECRETKEY_BYTES];
} kem_sk;

typedef struct {
    uint8_t pk[KEM_PUBLICKEY_BYTES];
} kem_pk;

typedef struct {
    uint8_t ct[KEM_CIPHERTXT_BYTES];
} kem_ct;

int kem_keygen(kem_sk *sk, kem_pk *pk);
int kem_encap(
    void *secret, size_t secret_len, kem_ct *ct,
    const kem_pk *pk);
int kem_decap(
    void *secret, size_t secret_len, const kem_ct *ct,
    const kem_sk *sk);

#endif




