#ifndef RSIG_API_H
#define RSIG_API_H

#include <stddef.h>
#include <stdint.h>

#define SIGN_PUBLICKEY_BYTES 896
#define SIGN_SECRETKEY_BYTES 2048
#define SIGN_SIGNATURE_BYTES 650
#define RSIG_PUBLICKEY_BYTES 1792
#define RSIG_SIGNATURE_BYTES 1276

#define COMPRESSED_SIGN_SIGNATURE_BYTES 626
#define SALT_BYTES 24
#define RING_K 2

typedef struct { double v; } fpr;

typedef struct{
  fpr coeffs[512];
} fpoly;

typedef struct{
  int32_t coeffs[512];
} poly;

typedef struct{
    int8_t f[512];
    int8_t g[512];
    int8_t F[512];
    int8_t G[512];
    poly b10;
    poly b11;
    poly b20;
    poly b21;
    fpoly GSO_b10; //~b1[0]/<~b1, ~b1>
    fpoly GSO_b11; //~b1[1]/<~b1, ~b1>
    fpoly GSO_b20; //~b2[0]/<~b2, ~b2>
    fpoly GSO_b21; //~b2[1]/<~b2, ~b2>
    fpoly beta10;
    fpoly beta11;
    fpoly beta20;
    fpoly beta21;
    fpoly sigma1;
    fpoly sigma2;
} sign_expanded_sk;

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

int sign_keygen(sign_sk *sk, sign_pk *pk);
void expand_sign_sk(sign_expanded_sk *expanded_sk, const sign_sk *sk);
int sign_keygen_expanded_sk(sign_expanded_sk *sk, sign_pk *pk);

void Gandalf_sign(rsig_signature *s, const uint8_t *m, const size_t mlen, const rsig_pk *pks,
    const sign_sk *sk, size_t party_id);
void Gandalf_sign_expanded_sk(rsig_signature *s, const uint8_t *m, const size_t mlen, const rsig_pk *pks,
    const sign_expanded_sk *expanded_sk, size_t party_id);
int Gandalf_verify(const uint8_t *m, const size_t mlen, const rsig_signature *s, const rsig_pk *pks);

#endif

