
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "kem.h"
#include "randombytes.h"

#define ITERATIONS 2048

int main(void) {

    uint8_t pk[CRYPTO_PUBLICKEYBYTES];
    uint8_t sk[CRYPTO_SECRETKEYBYTES];
    uint8_t ct[CRYPTO_CIPHERTEXTBYTES];
    uint8_t key_a[CRYPTO_BYTES];
    uint8_t key_b[CRYPTO_BYTES];

    // 32 <= slen <= 64
    const size_t slen = 64;

    int correct;

    correct = 0;
    for(int i = 0; i < ITERATIONS; i++){

        crypto_kem_keypair(pk, sk);
        crypto_kem_enc(ct, key_b, slen, pk);
        crypto_kem_dec(key_a, slen, ct, sk);
        correct += (memcmp(key_a, key_b, slen) == 0);

    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == ITERATIONS)?"ok":"ERROR!");

    correct = 0;
    for(int i = 0; i < ITERATIONS; i++){

        crypto_kem_keypair(pk, sk);
        crypto_kem_enc(ct, key_b, slen, pk);
        randombytes(sk, CRYPTO_SECRETKEYBYTES);
        crypto_kem_dec(key_a, slen, ct, sk);
        correct += (memcmp(key_a, key_b, slen) == 0);

    }
    printf("%d/%d compatible shared secret pairs (invalid secret key). (%s).\n\n", correct, ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

    correct = 0;
    for(int i = 0; i < ITERATIONS; i++){

        crypto_kem_keypair(pk, sk);
        crypto_kem_enc(ct, key_b, slen, pk);
        ct[rand() % CRYPTO_CIPHERTEXTBYTES] ^= 0xff;
        crypto_kem_dec(key_a, slen, ct, sk);
        correct += (memcmp(key_a, key_b, slen) == 0);

    }
    printf("%d/%d compatible shared secret pairs (ciphertext with a randomly toggled byte). (%s).\n\n", correct, ITERATIONS,
        (correct == 0)?"ok":"ERROR!");


}

