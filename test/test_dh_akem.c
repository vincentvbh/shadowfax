
#include "dh_akem_api.h"
#include "randombytes.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#define ITERATIONS 2048

int main(void){

    nike_sk sender_sk, receiver_sk, attacker_sk;
    nike_pk sender_pk, receiver_pk, attacker_pk;
    nike_pk ct;
    uint8_t sender_secret[32], receiver_secret[32], attacker_secret[32];

    int correct;

    // initialize randombyte seed
    seed_rng();

// NIKE AKEM

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        nike_akem_keygen(&sender_sk, &sender_pk);
        nike_akem_keygen(&receiver_sk, &receiver_pk);

        nike_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &receiver_pk);
        nike_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk);

        correct += (memcmp(sender_secret, receiver_secret, 32) == 0);
        assert(correct == (i + 1));
    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == ITERATIONS)?"ok":"ERROR!");


    nike_akem_keygen(&sender_sk, &sender_pk);
    nike_akem_keygen(&receiver_sk, &receiver_pk);

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        nike_akem_keygen(&attacker_sk, &attacker_pk);
        nike_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &attacker_pk);
        nike_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk);

        correct += memcmp(sender_secret, receiver_secret, 32) == 0;
        assert(correct == 0);
    }
    printf("%d/%d success decapsulation + compatible shared secret pairs. (%s).\n\n", correct, 2 * ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        nike_akem_keygen(&attacker_sk, &attacker_pk);
        nike_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &receiver_pk);
        nike_akem_decap(attacker_secret, &ct, &attacker_sk, &attacker_pk, &sender_pk);

        correct += memcmp(sender_secret, attacker_secret, 32) == 0;
        assert(correct == 0);
    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

}



