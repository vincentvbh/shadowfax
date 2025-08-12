
#include "rsig_keygen_helper.h"
#include "inner.h"
#include "kgen_inner.h"
#include "randombytes.h"

void RSIG_keygen(sign_sk *sk, sign_pk *pk) {

    uint8_t seed[32];

    int8_t f[N], g[N];

    randombytes(seed, 32);

    shake_context pc;
    shake_init(&pc, 256);
    shake_inject(&pc, seed, 32);
    shake_flip(&pc);

    sample_f(LOG_N, &pc, f);
    sample_f(LOG_N, &pc, g);


}
