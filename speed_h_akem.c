

#include "randombytes.h"
#include "h_akem_api.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if __APPLE__
#define __AVERAGE__
#else
#define __MEDIAN__
#endif
#include "cycles.h"

#define NTESTS 2048
uint64_t time0, time1;
uint64_t cycles[NTESTS];

int main(void){

    h_akem_sk sender_sk, receiver_sk;
    h_akem_expanded_sk sender_expanded_sk;
    h_akem_pk sender_pk, receiver_pk;
    h_akem_ct ct;
    nike_s s;
    rsig_pk internal_rsig_pk;
    rsig_signature internal_signature;
    uint8_t sender_secret[32], receiver_secret[32];
    uint8_t kk[32];
    uint8_t m[NTESTS][MLEN];

    // initialize randombyte seed
    init_prng();

    // initialize performance counter
    init_counter();

// ========
// akem operations

    WRAP_FUNC("h_akem_keygen_expanded_sk",
              cycles, time0, time1,
              h_akem_keygen_expanded_sk(&sender_expanded_sk, &sender_pk));

    WRAP_FUNC("h_akem_keygen",
              cycles, time0, time1,
              h_akem_keygen(&sender_sk, &sender_pk));

    WRAP_FUNC("h_akem_encap_expanded_sk",
              cycles, time0, time1,
              h_akem_encap_expanded_sk(sender_secret, &ct, &sender_expanded_sk, &sender_pk, &receiver_pk));

    WRAP_FUNC("h_akem_encap",
              cycles, time0, time1,
              h_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &receiver_pk));

    WRAP_FUNC("h_akem_decap",
              cycles, time0, time1,
              h_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk));

// ========
// nike operations

    WRAP_FUNC("nike_keygen",
              cycles, time0, time1,
              nike_keygen(&sender_sk.nsk, &sender_pk.npk));

    WRAP_FUNC("nike_sdk",
              cycles, time0, time1,
              nike_sdk(&s, &sender_sk.nsk, &receiver_pk.npk));

// ========
// kem operations

    WRAP_FUNC("kem_keygen",
              cycles, time0, time1,
              kem_keygen(&sender_sk.ksk, &sender_pk.kpk));

    WRAP_FUNC("kem_encap",
              cycles, time0, time1,
              kem_encap(kk, 64, &ct.ct, &receiver_pk.kpk));

    WRAP_FUNC("kem_decap",
              cycles, time0, time1,
              kem_decap(kk, 64, &ct.ct, &receiver_sk.ksk));

// ========
// ring signature operations

    WRAP_FUNC("sign_keygen_expanded_sk",
              cycles, time0, time1,
              sign_keygen_expanded_sk(&sender_expanded_sk.ssk, &sender_pk.spk));

    WRAP_FUNC("sign_keygen",
              cycles, time0, time1,
              sign_keygen(&sender_sk.ssk, &sender_pk.spk));

    for(size_t i = 0; i < NTESTS; i++){
        randombytes(m[i], MLEN);
    }

    sign_keygen_expanded_sk(&sender_expanded_sk.ssk, &sender_pk.spk);
    sign_keygen(&receiver_sk.ssk, &receiver_pk.spk);
    internal_rsig_pk.hs[0] = sender_pk.spk;
    internal_rsig_pk.hs[0] = receiver_pk.spk;

    WRAP_FUNC("Gandalf_sign_expanded_sk",
              cycles, time0, time1,
              Gandalf_sign_expanded_sk(&internal_signature, m[i], MLEN, &internal_rsig_pk, &sender_expanded_sk.ssk, 0));

    sign_keygen(&sender_sk.ssk, &sender_pk.spk);
    WRAP_FUNC("Gandalf_sign",
              cycles, time0, time1,
              Gandalf_sign(&internal_signature, m[i], MLEN, &internal_rsig_pk, &sender_sk.ssk, 0));

    WRAP_FUNC("Gandalf_verify",
              cycles, time0, time1,
              Gandalf_verify(m[i], MLEN, &internal_signature, &internal_rsig_pk));

    return 0;

}






