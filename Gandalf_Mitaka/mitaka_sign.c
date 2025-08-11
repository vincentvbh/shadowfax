
#include "mitaka_sign.h"
#include "randombytes.h"
#include "mitaka_sampler.h"
#include "fips202.h"
#include "hash.h"
#include "pack_unpack.h"
#include "encode_decode.h"

static
int poly_check_norm(const poly *p1, const poly *p2){

    int32_t acc;

    acc = 0;

    for(size_t i = 0; i < N; i++){
        acc += p1->coeffs[i] * p1->coeffs[i];
        if(acc >= MITAKA_BOUND_SQUARE_FLOOR + 1){
            acc = MITAKA_BOUND_SQUARE_FLOOR + 1;
        }
    }
    for(size_t i = 0; i < N; i++){
        acc += p2->coeffs[i] * p2->coeffs[i];
        if(acc >= MITAKA_BOUND_SQUARE_FLOOR + 1){
            acc = MITAKA_BOUND_SQUARE_FLOOR + 1;
        }
    }

    return acc <= MITAKA_BOUND_SQUARE_FLOOR;

}


void Mitaka_sign_expanded_sk(sign_signature *s, const uint8_t *m, const size_t mlen, const sign_expanded_sk *sk){

    poly c2, v0, v1;
    uint8_t salt[SALT_BYTES];

    shake128incctx state_init, state_dyn;

    shake128_inc_init(&state_init);
    shake128_inc_absorb(&state_init, m, mlen);

    do {

        randombytes(salt, SALT_BYTES);

        shake128_inc_ctx_clone(&state_dyn, &state_init);
        shake128_inc_absorb(&state_dyn, salt, SALT_BYTES);
        shake128_inc_finalize(&state_dyn);
        hash_to_poly(&c2, &state_dyn);

        sampler(&v0, &v1, sk, c2);

    } while (!poly_check_norm(&v0, &v1));

    compress_u_from_poly(&s->compressed_sign, v0.coeffs);

    for(size_t i = 0; i < SALT_BYTES; i++){
        (s->salt)[i] = salt[i];
    }

}


int Mitaka_verify(const uint8_t *m, const size_t mlen, const sign_pk *pk, const sign_signature *s){

    poly s2, h_poly, c1, s1;
    shake128incctx state;

    shake128_inc_init(&state);
    shake128_inc_absorb(&state, m, mlen);
    shake128_inc_absorb(&state, s->salt, SALT_BYTES);
    shake128_inc_finalize(&state);
    hash_to_poly(&c1, &state);

    unpack_h(&h_poly, pk->h);

    decompress_u_to_poly(s1.coeffs, s->compressed_sign);

    // compute s2 = c1 - s1 * h
    poly_mul(&s2, &s1, &h_poly);
    poly_sub(&s2, &c1, &s2);
    poly_freeze(&s2, &s2);

    return poly_check_norm(&s1, &s2);

}

