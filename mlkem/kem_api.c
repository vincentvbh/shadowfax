
#include "kem_api.h"
#include "kem.h"

int kem_keygen(kem_sk *sk, kem_pk *pk) {
    crypto_kem_keypair(pk->pk, sk->sk);
    return 1;
}

int kem_encap(
    void *secret, size_t secret_len, kem_ct *ct,
    const kem_pk *pk) {
    crypto_kem_enc(ct->ct, secret, secret_len, pk->pk);
    return 1;
}

int kem_decap(
    void *secret, size_t secret_len, const kem_ct *ct,
    const kem_sk *sk) {
    crypto_kem_dec(secret, secret_len, ct->ct, sk->sk);
    return 1;
}
