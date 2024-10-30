#ifndef COMPUTE_KEYS_H
#define COMPUTE_KEYS_H

#include "rsig_params.h"
#include "expanded_keys.h"

#include <stdint.h>
#include <stdbool.h>

bool compute_public(uint8_t *h, const int8_t *f, const int8_t *g);
void expand_sign_sk(sign_expanded_sk *expanded_sk, const sign_sk *sk);

void compute_GSO(sign_expanded_sk* sk);
void compute_sigma(sign_expanded_sk* sk);
void compute_beta_hat(sign_expanded_sk* sk);

#endif

