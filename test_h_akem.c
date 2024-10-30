
#include "h_akem_api.h"
#include "randombytes.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ITERATIONS 2048

int main(void){

    h_akem_sk sender_sk, receiver_sk, attacker_sk;
    h_akem_expanded_sk sender_expanded_sk;
    h_akem_pk sender_pk, receiver_pk, attacker_pk;
    h_akem_ct ct;
    uint8_t sender_secret[32], receiver_secret[32], attacker_secret[32];

    int correct;

    // initialize randombyte seed
    seed_rng();

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        h_akem_keygen(&sender_sk, &sender_pk);
        h_akem_keygen(&receiver_sk, &receiver_pk);
        h_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &receiver_pk);
        correct += (h_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk) == 1) &&
                   (memcmp(sender_secret, receiver_secret, 32) == 0);
    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == ITERATIONS)?"ok":"ERROR!");

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        h_akem_keygen_expanded_sk(&sender_expanded_sk, &sender_pk);
        h_akem_keygen(&receiver_sk, &receiver_pk);
        h_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &receiver_pk);
        correct += (h_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk) == 1) &&
                   (memcmp(sender_secret, receiver_secret, 32) == 0);

    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == ITERATIONS)?"ok":"ERROR!");

    h_akem_keygen_expanded_sk(&sender_expanded_sk, &sender_pk);
    h_akem_keygen(&receiver_sk, &receiver_pk);

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        h_akem_keygen(&attacker_sk, &attacker_pk);
        h_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &attacker_pk);

        correct += h_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk) == 1;
        correct += memcmp(sender_secret, receiver_secret, 32) == 0;

    }
    printf("%d/%d success decapsulation + compatible shared secret pairs. (%s).\n\n", correct, 2 * ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

    correct = 0;
    for(size_t i = 0; i < ITERATIONS; i++){

        h_akem_keygen(&attacker_sk, &attacker_pk);
        h_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &receiver_pk);

        h_akem_decap(attacker_secret, &ct, &attacker_sk, &attacker_pk, &sender_pk);
        correct += memcmp(sender_secret, attacker_secret, 32) == 0;

    }
    printf("%d/%d compatible shared secret pairs. (%s).\n\n", correct, ITERATIONS,
        (correct == 0)?"ok":"ERROR!");

}




