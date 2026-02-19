#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include <stddef.h>

void seed_rng(void);

int randombytes(uint8_t *buf, size_t n);
uint64_t get64(void);
uint8_t get8(void);
void init_prng(void);


#endif
