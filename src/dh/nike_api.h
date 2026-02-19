#ifndef NIKE_API_H
#define NIKE_API_H

#include <stdint.h>

#define NIKE_PUBLICKEY_BYTES 32
#define NIKE_SECRETKEY_BYTES 32
#define NIKE_BYTES 64

typedef struct {
    uint8_t sk[NIKE_SECRETKEY_BYTES];
} nike_sk;

typedef struct {
    uint8_t pk[NIKE_PUBLICKEY_BYTES];
} nike_pk;

typedef struct {
    uint8_t s[NIKE_BYTES];
} nike_s;

int nike_keygen(nike_sk *sk, nike_pk *pk);
int nike_sdk(nike_s *s, const nike_sk *sk, const nike_pk *pk);

#endif

