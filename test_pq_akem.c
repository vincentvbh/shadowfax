

#include "randombytes.h"
#include "pq_akem_api.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <memory.h>
#include <assert.h>

#define ITERATIONS 2048

int main(){

    pq_akem_sk sender_sk, receiver_sk, attacker_sk;
    pq_akem_expanded_sk sender_expanded_sk;
    pq_akem_pk sender_pk, receiver_pk, attacker_pk;
    pq_akem_ct ct;
    uint8_t sender_secret[32], receiver_secret[32], attacker_secret[32];

    int correct;

    // initialize randombyte seed
    seed_rng();

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        pq_akem_keygen(&sender_sk, &sender_pk);
        pq_akem_keygen(&receiver_sk, &receiver_pk);
        pq_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &receiver_pk);

        correct += (pq_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk) == 1) &&
                   (memcmp(sender_secret, receiver_secret, 32) == 0);

    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == ITERATIONS)?"ok":"ERROR!");

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        pq_akem_keygen_expanded_sk(&sender_expanded_sk, &sender_pk);
        pq_akem_keygen(&receiver_sk, &receiver_pk);
        pq_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &receiver_pk);

        correct += (pq_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk) == 1) &&
                   (memcmp(sender_secret, receiver_secret, 32) == 0);


    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == ITERATIONS)?"ok":"ERROR!");

    pq_akem_keygen_expanded_sk(&sender_expanded_sk, &sender_pk);
    pq_akem_keygen(&receiver_sk, &receiver_pk);

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        pq_akem_keygen(&attacker_sk, &attacker_pk);
        pq_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &attacker_pk);

        correct += pq_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk) == 1;
        correct += memcmp(sender_secret, receiver_secret, 32) == 0;

    }
    printf("%d/%d success decapsulation + compatible shared secret pairs. (%s).\n\n", correct, 2 * ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        pq_akem_keygen(&attacker_sk, &attacker_pk);
        pq_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &receiver_pk);

        pq_akem_decap(attacker_secret, &ct, &attacker_sk, &attacker_pk, &sender_pk);
        correct += memcmp(sender_secret, attacker_secret, 32) == 0;

    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

    return 0;

}


