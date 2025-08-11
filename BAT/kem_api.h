#ifndef KEM_API_H
#define KEM_API_H

// BAT_257_512

#include <stddef.h>
#include <stdint.h>

// 1 (TAGBYTE) + N / 64 * 65 (encode_257)
#define KEM_PUBLICKEY_BYTES 521
// 1 (TAGBYTE) + N / 64 * 57 (encode_ciphtertext_257) + 16 (C2_BYTES)
#define KEM_CIPHERTXT_BYTES 473

#define KEM_SECRETKEY_BYTES 2953
#define KEM_SHORTSECRETKEY_BYTES 417

#define C2_BYTES 16
#define SEED_BYTES 32

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
int kem_encap_seed(
    void *secret, size_t secret_len, kem_ct *ct,
    const kem_pk *pk, const void *m);
int kem_decap(
    void *secret, size_t secret_len, const kem_ct *ct,
    const kem_sk *sk);

#endif

