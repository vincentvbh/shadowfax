
#include "sign_inner.h"

#include "gandalf_samplerZ.h"
#include "rsig.h"
#include "hash.h"
#include "randombytes.h"
#include "pack_unpack.h"
#include "encode_decode.h"

#include <memory.h>
#include <assert.h>

#include <stdio.h>

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

static
void sampler(poly *u, poly *v, const sign_sk *sk, const poly c, const poly h) {
    fpr tmp[7 * N];
    uint8_t seed[56];
    // s1 -> v, s2 -> u
    int16_t s1[N], s2[N];
    uint16_t c_buff[N];
    sampler_state ss;

    randombytes(seed, 56);
    sampler_init(&ss, LOG_N, seed, 56);

    for(size_t i = 0; i < N; i++){
        c_buff[i] = (uint16_t)c.coeffs[i];
    }
    trapdoor_sampler(LOG_N, s1, s2, sk->f, sk->g, sk->F, sk->G, c_buff, seed, tmp);
    // c = v + h * u
    for(size_t i = 0; i < N; i++){
        u->coeffs[i] = (int32_t)s2[i];
        v->coeffs[i] = (int32_t)s1[i];
    }


}

void Gandalf_sign(rsig_signature *s, const uint8_t *m, const size_t mlen,
        const rsig_pk *pks, const sign_sk *sk, size_t party_id){

    poly hash, h_poly;
    poly v, acc;
    poly u[RING_K];
    poly c[RING_K];
    int32_t mask;

    uint8_t salt[SALT_BYTES];
    shake128incctx state;

    unpack_h(&h_poly, &(pks->hs[party_id]).h[0]);

    do {

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

        // c[party_id] = hash - h[!party_id] * u[!party_id]
        poly_sub(c + party_id, &hash, &acc);
        poly_freeze(c + party_id, c + party_id);

        for(size_t i = 0; i < N; i++){

            mask = c[party_id].coeffs[i];
            mask = -((mask >> 31) & 1);
            c[party_id].coeffs[i] += (Q & mask);

            mask = ((Q - 1) - c[party_id].coeffs[i]);
            mask = -((mask >> 31) & 1);
            c[party_id].coeffs[i] -= (Q & mask);

            assert( (0 <= c[party_id].coeffs[i]) && (c[party_id].coeffs[i] < Q) );
        }

        unpack_h(&h_poly, &(pks->hs[party_id]).h[0]);

        // c[party_id] = v + h[party_id] * u[party_id]
        // hash = v + h[!party_id] * u[!party_id] + h[party_id] * u[party_id]
        sampler(u + party_id, &v, sk, c[party_id], h_poly);

    } while(Gandalf_signature_check_norm(u, v) == 0);

    // assert(Gandalf_signature_check_norm(u, v) == 1);

    for(size_t i = 0; i < RING_K; i++){
        compress_u_from_poly(&s->compressed_sign[i], u[i].coeffs);
    }

    memmove(&s->salt, salt, SALT_BYTES);

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

