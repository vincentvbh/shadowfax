#ifndef GANDALF_SAMPLERZ_H
#define GANDALF_SAMPLERZ_H

#include "poly.h"

#include <stdint.h>

#define GANDALF_TABLE_SIZE (1308)

extern
uint64_t Gandalf_table[GANDALF_TABLE_SIZE];

int32_t Gandalf_Gaussian_sampler();
void Gandalf_sample_poly(poly *u);

#endif

