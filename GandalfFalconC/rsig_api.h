#ifndef RSIG_API_H
#define RSIG_API_H

#include <stddef.h>
#include <stdint.h>

// TODO: update here
#define SIGN_PUBLICKEY_BYTES 896
#define SIGN_SECRETKEY_BYTES 2048
#define SIGN_SIGNATURE_BYTES 650
#define RSIG_PUBLICKEY_BYTES 1792
#define RSIG_SIGNATURE_BYTES 1276

#define COMPRESSED_SIGN_SIGNATURE_BYTES 626
#define SALT_BYTES 24
#define RING_K 2

typedef struct{
  int32_t coeffs[512];
} poly;

// N = 512
typedef struct{
    int8_t f[512];
    int8_t g[512];
    int8_t F[512];
    int8_t G[512];
} sign_sk;

// N = 512
typedef struct{
    uint8_t h[896];
} sign_pk;

typedef struct{
    uint8_t compressed_sign[COMPRESSED_SIGN_SIGNATURE_BYTES];
    uint8_t salt[SALT_BYTES];
} sign_signature;

typedef struct {
    sign_pk hs[RING_K];
} rsig_pk;

typedef struct {
    uint8_t compressed_sign[RING_K][COMPRESSED_SIGN_SIGNATURE_BYTES];
    uint8_t salt[SALT_BYTES];
} rsig_signature;

void sign_keygen(sign_sk *sk, sign_pk *pk);
void Gandalf_sign(rsig_signature *s, const uint8_t *m, const size_t mlen, const rsig_pk *pks,
    const sign_sk *sk, size_t party_id);
int Gandalf_verify(const uint8_t *m, const size_t mlen, const rsig_signature *s, const rsig_pk *pks);

#endif

