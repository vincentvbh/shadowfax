
#include "kem_api.h"
#include "kem.h"
#include "fips202.h"

int kem_keygen(kem_sk *sk, kem_pk *pk) {
    crypto_kem_keypair(pk->pk, sk->sk);
    return 1;
}

int kem_encap(
    void *secret, size_t secret_len, kem_ct *ct,
    const kem_pk *pk) {
    uint8_t key[CRYPTO_BYTES];
    crypto_kem_enc(ct->ct, key, pk->pk);
    shake256(secret, secret_len, key, CRYPTO_BYTES);
    return 1;
}

int kem_decap(
    void *secret, size_t secret_len, const kem_ct *ct,
    const kem_sk *sk) {

    uint8_t key[CRYPTO_BYTES];

    crypto_kem_dec(key, ct->ct, sk->sk);
    shake256(secret, secret_len, key, CRYPTO_BYTES);

    // crypto_kem_dec(secret, secret_len, ct->ct, sk->sk);
    return 1;
}
