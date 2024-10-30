#ifndef RSIG_H
#define RSIG_H


#include "rsig_params.h"
#include <stddef.h>

int32_t Gandalf_HalfGaussian_sampler();
int32_t Gandalf_Gaussian_sampler();

void Gandalf_sign(rsig_signature *s, const uint8_t *m, const size_t mlen, const rsig_pk *pks,
        const sign_sk *sk, size_t party_id);
void Gandalf_sign_expanded_sk(rsig_signature *s, const uint8_t *m, const size_t mlen, const rsig_pk *pks,
        const sign_expanded_sk *expanded_sk, size_t party_id);
int Gandalf_verify(const uint8_t *m, const size_t mlen, const rsig_signature *s, const rsig_pk *pks);

#endif

