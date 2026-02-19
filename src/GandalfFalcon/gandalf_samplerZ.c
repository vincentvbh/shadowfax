
#include "rsig_params.h"
#include "gandalf_samplerZ.h"
#include "randombytes.h"

#include <math.h>
#include <stddef.h>

#include "samplerZ_table.c"

int32_t Gandalf_Gaussian_sampler(){

    uint64_t zero_mask, neg_mask, r;
    int32_t z;

    zero_mask = get64();
    // when zero_mask = -1,
    //     neg_mask = 0 if we return a positive value
    //     neg_mask = -1 if we return a negative value
    neg_mask = -(zero_mask >> 63);

    // zero_mask = 0 if we return 0
    // zero_mask = -1 if we return a non-zero value
    zero_mask &= (1ULL << 63) - 1;
    zero_mask = ((zero_mask - Gandalf_table[0]) >> 63) & 1;
    zero_mask = ~(-zero_mask);

    r = get64();

    z = 1;
    for(size_t i = 1; i < GANDALF_TABLE_SIZE; i++){
        z += (r >= Gandalf_table[i]);
    }

    z = ( z & (~neg_mask) ) | ( (-z) & (neg_mask) );

    return zero_mask & z;

}

void Gandalf_sample_poly(poly *u){
    for(size_t i = 0; i < N; i++){
        u->coeffs[i] = Gandalf_Gaussian_sampler();
    }
}

