
#include "rng.h"
#include "sys_rand.h"
#include "randombytes.h"

prng p;

int randombytes(uint8_t *buf, size_t n){
    return prng_get_bytes(&p, buf, (int)n);
}

void randombytes_init(uint8_t *seed, size_t seed_len){
    prng_init(&p, seed, seed_len, 0);
}

uint64_t get64(){
    return prng_get_u64(&p);
}

uint8_t get8(){
    return prng_get_u8(&p);
}

void seed_rng(void){
    // must not exceed 48 bytes
    uint8_t seed[48];

    get_seed(seed, sizeof seed);

    prng_init(&p, seed, sizeof seed, 0);
}

