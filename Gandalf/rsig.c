
#include "compute_keys.h"
#include "rsig.h"
#include "expanded_keys.h"
#include "mitaka_sampler.h"
#include "gandalf_samplerZ.h"
#include "hash.h"
#include "randombytes.h"

#include "pack_unpack.h"
#include "encode_decode.h"

#include <memory.h>
#include <assert.h>

static
int Gandalf_signature_check_norm(const poly u[RING_K], const poly v){

    int32_t acc;

    acc = 0;

    for(size_t i = 0; i < RING_K; i++){
        for(size_t j = 0; j < N; j++){
            acc += u[i].coeffs[j] * u[i].coeffs[j];
            if(acc >= GANDALF_BOUND_SQUARE_FLOOR + 1){
                acc = GANDALF_BOUND_SQUARE_FLOOR + 1;
            }
        }
    }

    for(size_t j = 0; j < N; j++){
        acc += v.coeffs[j] * v.coeffs[j];
        if(acc >= GANDALF_BOUND_SQUARE_FLOOR + 1){
            acc = GANDALF_BOUND_SQUARE_FLOOR + 1;
        }
    }

    return acc <= GANDALF_BOUND_SQUARE_FLOOR;

}

void Gandalf_sign_expanded_sk(rsig_signature *s, const uint8_t *m, const size_t mlen,
        const rsig_pk *pks, const sign_expanded_sk *expanded_sk, size_t party_id){

    poly hash, h_poly;
    poly v, acc;
    poly u[RING_K];
    poly c[RING_K];

    uint8_t salt[SALT_BYTES];
    shake128incctx state;

    randombytes(salt, SALT_BYTES);

    shake128_inc_init(&state);
    shake128_inc_absorb(&state, (uint8_t*)m, mlen);
    shake128_inc_absorb(&state, (uint8_t*)pks, RSIG_PUBLICKEY_BYTES);
    shake128_inc_absorb(&state, (uint8_t*)salt, SALT_BYTES);
    shake128_inc_finalize(&state);
    hash_to_poly(&hash, &state);

    memset(&acc, 0, sizeof(acc));
    for(size_t i = 0; i < RING_K; i++){
        if(i == party_id)
            continue;
        Gandalf_sample_poly(u + i);
        unpack_h(&h_poly, &(pks->hs[i]).h[0]);
        poly_mul(c + i, u + i, &h_poly);
        poly_add(&acc, &acc, c + i);
    }

    poly_sub(c + party_id, &hash, &acc);
    poly_freeze(c + party_id, c + party_id);

    sampler(u + party_id, &v, expanded_sk, c[party_id]);

    // assert(Gandalf_signature_check_norm(u, v) == 1);

    for(size_t i = 0; i < RING_K; i++){
        compress_u_from_poly(&s->compressed_sign[i], u[i].coeffs);
    }

    memmove(&s->salt, salt, SALT_BYTES);

}

void Gandalf_sign(rsig_signature *s, const uint8_t *m, const size_t mlen,
        const rsig_pk *pks, const sign_sk *sk, size_t party_id){

    sign_expanded_sk expanded_sk;

    expand_sign_sk(&expanded_sk, sk);
    Gandalf_sign_expanded_sk(s, m, mlen, pks, &expanded_sk, party_id);

}

int Gandalf_verify(const uint8_t *m, const size_t mlen, const rsig_signature *s, const rsig_pk *pks){

    poly v, h_poly;
    poly prod, acc;
    poly u[RING_K];

    shake128incctx state;

    shake128_inc_init(&state);
    shake128_inc_absorb(&state, (uint8_t*)m, mlen);
    shake128_inc_absorb(&state, (uint8_t*)pks, RSIG_PUBLICKEY_BYTES);
    shake128_inc_absorb(&state, (uint8_t*)s->salt, SALT_BYTES);
    shake128_inc_finalize(&state);
    hash_to_poly(&v, &state);

    memset(&acc, 0, sizeof(acc));
    for(size_t i = 0; i < RING_K; i++){
        unpack_h(&h_poly, &(pks->hs[i]).h[0]);
        decompress_u_to_poly(u[i].coeffs, &s->compressed_sign[i]);
        poly_mul(&prod, &u[i], &h_poly);
        poly_add(&acc, &acc, &prod);
    }

    poly_sub(&v, &v, &acc);
    poly_freeze(&v, &v);

    return Gandalf_signature_check_norm(u, v);

}

