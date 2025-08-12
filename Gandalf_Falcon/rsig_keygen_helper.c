
#include "rsig_keygen_helper.h"
#include "inner.h"
#include "kgen_inner.h"
#include "randombytes.h"

void sign_keygen(sign_sk *sk, sign_pk *pk) {

    uint8_t seed[32];

    int8_t f[N], g[N];
    uint16_t h[N];
    __attribute__((aligned(32))) uint8_t tmp[26 * N];

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
        mqpoly_encode(LOG_N, h, pk->h);

        break;

    }

}




