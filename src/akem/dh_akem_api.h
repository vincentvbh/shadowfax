#ifndef NIKE_AKEM_API_H
#define NIKE_AKEM_API_H

#include "nike_api.h"

#define NIKE_AKEM_CIPHERTXT_BYTES 32
#define NIKE_AKEM_BYTES 32

void nike_akem_keygen(nike_sk *sk, nike_pk *pk);
void nike_akem_encap(uint8_t *k, nike_pk *ct,
            const nike_sk *sender_sk, const nike_pk *sender_pk, const nike_pk *receiver_pk);
void nike_akem_decap(uint8_t *k, const nike_pk *ct,
            const nike_sk *receiver_sk, const nike_pk *receiver_pk, const nike_pk *sender_pk);

#endif

