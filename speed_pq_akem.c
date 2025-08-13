

#include "randombytes.h"
#include "pq_akem_api.h"
#include "rsig_api.h"

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

int main(){

    pq_akem_sk sender_sk, receiver_sk;
    pq_akem_pk sender_pk, receiver_pk;
    pq_akem_ct ct;
    rsig_pk internal_rsig_pk;
    rsig_signature internal_signature;
    poly a, b, c;
    uint8_t sender_secret[32], receiver_secret[32];
    uint8_t kk[48];
    uint8_t m[NTESTS][MLEN];

    printf("Post-quantum AKEM public key bytes: %4zu\n", sizeof(pq_akem_pk));
    printf("Post-quantum AKEM secret key bytes: %4zu\n", sizeof(pq_akem_sk));
    printf("Post-quantum AKEM ciphertext bytes: %4zu\n", sizeof(pq_akem_ct));

    for(size_t i = 0; i < 512; i++){
      a.coeffs[i] = rand() % 12289;
      a.coeffs[i] -= 6144;
      b.coeffs[i] = rand() % 12289;
      b.coeffs[i] -= 6144;
      c.coeffs[i] = rand() % 12289;
      c.coeffs[i] -= 6144;
    }

    // initialize randombyte seed
    init_prng();

    // initialize performance counter
    init_counter();

// ========
// akem operations

    WRAP_FUNC("pq_akem_keygen",
              cycles, time0, time1,
              pq_akem_keygen(&sender_sk, &sender_pk));

    WRAP_FUNC("pq_akem_encap",
              cycles, time0, time1,
              pq_akem_encap(sender_secret, &ct, &sender_sk, &sender_pk, &receiver_pk));

    WRAP_FUNC("pq_akem_decap",
              cycles, time0, time1,
              pq_akem_decap(receiver_secret, &ct, &receiver_sk, &receiver_pk, &sender_pk));

// ========
// kem operations

    WRAP_FUNC("kem_keygen",
              cycles, time0, time1,
              kem_keygen(&sender_sk.ksk, &sender_pk.kpk));

    WRAP_FUNC("kem_encap",
              cycles, time0, time1,
              kem_encap(kk, 48, &ct.ct, &receiver_pk.kpk));

    WRAP_FUNC("kem_decap",
              cycles, time0, time1,
              kem_decap(kk, 48, &ct.ct, &receiver_sk.ksk));

// ========
// ring signature operations

    WRAP_FUNC("sign_keygen",
              cycles, time0, time1,
              sign_keygen(&sender_sk.ssk, &sender_pk.spk));

    sign_keygen(&sender_sk.ssk, &sender_pk.spk);
    WRAP_FUNC("Gandalf_sign",
              cycles, time0, time1,
              Gandalf_sign(&internal_signature, m[i], MLEN, &internal_rsig_pk, &sender_sk.ssk, 0));

    WRAP_FUNC("Gandalf_verify",
              cycles, time0, time1,
              Gandalf_verify(m[i], MLEN, &internal_signature, &internal_rsig_pk));

    return 0;

}


