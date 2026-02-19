
#include "nike_api.h"
#include "curve25519.h"
#include "fips202.h"

int nike_keygen(nike_sk *sk, nike_pk *pk){
    return dh_keypair(sk->sk, pk->pk);
}

int nike_sdk(nike_s *s, const nike_sk *sk, const nike_pk *pk){
    unsigned char buff[DH_BYTES];
    dh(buff, sk->sk, pk->pk);
    sha3_512((uint8_t*)s->s, buff, sizeof buff);
    return 0;
}

