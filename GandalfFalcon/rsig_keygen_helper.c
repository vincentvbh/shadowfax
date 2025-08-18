
#include "rsig_keygen_helper.h"
#include "inner.h"
#include "kgen_inner.h"
#include "randombytes.h"

#include <assert.h>
#include "poly.h"

#include "pack_unpack.h"

void sign_keygen(sign_sk *sk, sign_pk *pk) {

    uint8_t seed[32];

    int8_t f[N], g[N];
    uint16_t h[N];
    __attribute__((aligned(32))) uint8_t tmp[26 * N];

    poly f_poly, g_poly, h_poly;
    poly buff_poly;

    randombytes(seed, 32);

    shake_context pc;
    shake_init(&pc, 256);
    shake_inject(&pc, seed, 32);
    shake_flip(&pc);

    while(1){

        sample_f(LOG_N, &pc, f);
        sample_f(LOG_N, &pc, g);

        if(check_fg_norm(N, f, g)){
            continue;
        }

        /* f must be invertible modulo X^n+1 modulo q. */
        if (!mqpoly_is_invertible(LOG_N, f, (uint16_t*)tmp)) {
            continue;
        }

        /* (f,g) must have an acceptable orthogonalized norm. */
        if (!check_ortho_norm(LOG_N, f, g, (fxr*)tmp)) {
            continue;
        }

        /* Try to solve the NTRU equation. */
        if (!solve_NTRU(LOG_N, f, g, (uint32_t*)tmp)) {
            continue;
        }

        memmove(sk->f, f, N);
        memmove(sk->g, g, N);
        memmove(sk->F, tmp, N);
        memmove(sk->G, tmp + N, N);

        mqpoly_div_small(LOG_N, g, f, h, (uint16_t*)tmp);
        // mqpoly_encode(LOG_N, h, pk->h);

        for(size_t i = 0; i < N; i++){
            f_poly.coeffs[i] = (int32_t)f[i];
            g_poly.coeffs[i] = (int32_t)g[i];
            h_poly.coeffs[i] = (int32_t)h[i];
        }
        pack_h(pk->h, &h_poly);


        poly_freeze(&f_poly, &f_poly);
        poly_freeze(&g_poly, &g_poly);
        poly_freeze(&h_poly, &h_poly);
        poly_mul(&buff_poly, &h_poly, &f_poly);

        for(size_t i = 0; i < N; i++){
            assert(g_poly.coeffs[i] == buff_poly.coeffs[i]);
        }

        // g = h f mod (q, x^n + 1)
        // g F - f G = 0 mod (q, x^n + 1)

        break;

    }

}




