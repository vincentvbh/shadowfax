#ifndef CURVE25519_H
#define CURVE25519_H

#define DH_SECRETKEY_BYTES 32
#define DH_PUBLICKEY_BYTES 32
#define DH_BYTES 32

int dh_keypair(unsigned char *sk,unsigned char *pk);
int dh(unsigned char *s,const unsigned char *sk,const unsigned char *pk);

#endif

