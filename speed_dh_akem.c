
#include "dh_akem_api.h"
#include "randombytes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NTESTS 2048
uint64_t time0, time1;
uint64_t cycles[NTESTS];

#define __AVERAGE__
#include "cycles.h"

#ifdef __APPLE__
#define CYCLE_TYPE "%lld"
#else
#define CYCLE_TYPE "%ld"
#endif

// make sure stdio.h and stdlib.h are included.
#ifdef __AVERAGE__

#define LOOP_INIT(__clock0, __clock1) { \
    __clock0 = get_cycle(); \
}
#define LOOP_TAIL(__f_string, records, __clock0, __clock1) { \
    __clock1 = get_cycle(); \
    printf(__f_string, (__clock1 - __clock0) / NTESTS); \
}
#define BODY_INIT(__clock0, __clock1) {}
#define BODY_TAIL(records, __clock0, __clock1) {}

#elif defined(__MEDIAN__)

#define LOOP_INIT(__clock0, __clock1) {}
#define LOOP_TAIL(__f_string, records, __clock0, __clock1) { \
    qsort(records, sizeof(uint64_t), NTESTS, cmp_uint64); \
    printf(__f_string, records[NTESTS >> 1]); \
}
#define BODY_INIT(__clock0, __clock1) { \
    __clock0 = get_cycle(); \
}
#define BODY_TAIL(records, __clock0, __clock1) { \
    __clock1 = get_cycle(); \
    records[i] = __clock1 - __clock0; \
}

#else

#error Benchmarking mode undefined! Please define __AVERAGE__ or __MEDIAN__.

#endif

#define WRAP_FUNC(__f_string, records, __clock0, __clock1, func) { \
    LOOP_INIT(__clock0, __clock1); \
    for(size_t i = 0; i < NTESTS; i++){ \
        BODY_INIT(__clock0, __clock1); \
        func; \
        BODY_TAIL(records, __clock0, __clock1); \
    } \
    LOOP_TAIL(__f_string, records, __clock0, __clock1); \
}

int main(void){

    nike_sk sk1, sk2;
    nike_pk pk1, pk2;
    nike_pk ct;
    nike_s s1;
    uint8_t nike_akem_secret1[NIKE_BYTES], nike_akem_secret2[NIKE_BYTES];

    // initialize randombyte seed
    seed_rng();

    // initialize performance counter
    init_counter();

// ========
// NIKE AKEM

    nike_keygen(&sk2, &pk2);

    WRAP_FUNC("nike_akem_encap cycles: " CYCLE_TYPE "\n",
              cycles, time0, time1,
              nike_akem_encap(nike_akem_secret1, &ct, &sk1, &pk1, &pk2));

    WRAP_FUNC("nike_akem_decap cycles: " CYCLE_TYPE "\n",
              cycles, time0, time1,
              nike_akem_decap(nike_akem_secret2, &ct, &sk2, &pk2, &pk1));

// ========
// NIKE

    WRAP_FUNC("nike_keygen cycles: " CYCLE_TYPE "\n",
              cycles, time0, time1,
              nike_keygen(&sk1, &pk1));

    WRAP_FUNC("nike_sdk cycles: " CYCLE_TYPE "\n",
              cycles, time0, time1,
              nike_sdk(&s1, &sk2, &pk1));

}







