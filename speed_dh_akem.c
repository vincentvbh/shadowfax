
#include "dh_akem_api.h"
#include "randombytes.h"

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

    nike_sk sk1, sk2;
    nike_pk pk1, pk2;
    nike_pk ct;
    nike_s s1;
    uint8_t nike_akem_secret1[NIKE_BYTES], nike_akem_secret2[NIKE_BYTES];

    // initialize randombyte seed
    init_prng();

    // initialize performance counter
    init_counter();

// ========
// NIKE AKEM

    nike_keygen(&sk2, &pk2);

    WRAP_FUNC("nike_akem_encap",
              cycles, time0, time1,
              nike_akem_encap(nike_akem_secret1, &ct, &sk1, &pk1, &pk2));

    WRAP_FUNC("nike_akem_decap",
              cycles, time0, time1,
              nike_akem_decap(nike_akem_secret2, &ct, &sk2, &pk2, &pk1));

// ========
// NIKE

    WRAP_FUNC("nike_keygen",
              cycles, time0, time1,
              nike_keygen(&sk1, &pk1));

    WRAP_FUNC("nike_sdk",
              cycles, time0, time1,
              nike_sdk(&s1, &sk2, &pk1));

}







