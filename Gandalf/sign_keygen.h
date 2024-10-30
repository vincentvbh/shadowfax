#ifndef SIGN_KEYGEN_H
#define SIGN_KEYGEN_H

#include "rsig_params.h"
#include "expanded_keys.h"

int keygen_fg(sign_sk *sk);
int sign_keygen(sign_sk *sk, sign_pk *pk);
int sign_keygen_expanded_sk(sign_expanded_sk *expanded_sk, sign_pk *pk);

#endif

