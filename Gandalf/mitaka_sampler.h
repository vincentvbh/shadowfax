#ifndef MITAKA_SAMPLER_H
#define MITAKA_SAMPLER_H

#include "expanded_keys.h"

#define TABLE_SIZE 13

extern
uint64_t CDT[TABLE_SIZE];

void normaldist(fpoly *vec);
int samplerZ(double u);
void sample_discrete_gauss(poly *des, const fpoly *src);
void sampler(poly *v0_ptr, poly *v1_ptr, const sign_expanded_sk *sk, const poly c2);

#endif

