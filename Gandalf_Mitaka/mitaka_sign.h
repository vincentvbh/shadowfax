#ifndef MITAKA_SIGN_H
#define MITAKA_SIGN_H

#include "rsig_params.h"
#include "expanded_keys.h"

#include <stddef.h>

void Mitaka_sign_expanded_sk(sign_signature *s, const uint8_t *m, const size_t mlen, const sign_expanded_sk *sk);
int Mitaka_verify(const uint8_t *m, const size_t mlen, const sign_pk *pk, const sign_signature *s);

#endif

