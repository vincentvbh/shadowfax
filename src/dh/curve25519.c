
#include "curve25519.h"
#include "scalarmult.h"
#include "randombytes.h"

int dh_keypair(unsigned char *sk,unsigned char *pk)
{
  randombytes(sk,DH_SECRETKEY_BYTES);
  scalarmult_base(pk,sk);
  return 0;
}

int dh(unsigned char *s,const unsigned char *sk,const unsigned char *pk)
{
  scalarmult(s,sk,pk);
  return 0;
}
