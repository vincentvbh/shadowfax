
#include "rsig_api.h"
#include "sign_keygen.h"
#include "compute_keys.h"
#include "mitaka_sign.h"
#include "randombytes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>  
#include <math.h>  
#include <memory.h>
#include <assert.h>

#define MAXMBYTES 2048
#define ITERATIONS 2048
#define MAX(x, y) ((x >= y)? (x):(y))
#define MIN(x, y) ((x < y)? (x):(y))

int intcmp(const void *x, const void *y)
{
    int ix = *(int*)x, iy = *(int*)y;
    return (ix>iy) - (ix<iy);
}

int main(){
  srand(time(0));
  seed_rng();
  sign_expanded_sk expanded_sk[16];
  sign_sk sk[16];
  sign_pk pk[16];
  sign_signature s;
  rsig_pk pks;
  rsig_signature Gandalf_s;
  size_t party_id;
  int correct;

  uint8_t m[MAXMBYTES] = {0x46,0xb6,0xc4,0x83,0x3f,0x61,0xfa,0x3e,0xaa,0xe9,0xad,0x4a,0x68,0x8c,0xd9,0x6e,0x22,0x6d,0x93,0x3e,0xde,0xc4,0x64,0x9a,0xb2,0x18,0x45,0x2,0xad,0xf3,0xc,0x61};

  printf("\n==== generate key pairs ====\n");

  printf("SIGN_PUBLICKEY_BYTES: %d\n", SIGN_PUBLICKEY_BYTES);
  printf("SIGN_SECRETKEY_BYTES: %d\n", SIGN_SECRETKEY_BYTES);
  printf("SIGN_SIGNATURE_BYTES: %d\n", SIGN_SIGNATURE_BYTES);
  printf("RSIG_PUBLICKEY_BYTES: %d\n", RSIG_PUBLICKEY_BYTES);
  printf("RSIG_SIGNATURE_BYTES: %d\n", RSIG_SIGNATURE_BYTES);
  printf("COMPRESSED_SIGN_SIGNATURE_BYTES: %d\n", COMPRESSED_SIGN_SIGNATURE_BYTES);

  assert(RING_K < 16);

  for(size_t i = 0; i < 16; i++){
    sign_keygen_expanded_sk(expanded_sk + i, pk + i);
    sign_keygen(sk + i, pk + i);
    expand_sign_sk(expanded_sk + i, sk + i);
  }

  for(size_t i = 0; i < RING_K; i++){
    pks.hs[i] = pk[i];
  }

  printf("\nkey pair generations done\n");

  printf("* Test correctness of Antrag.\n");

  correct = 0;
  for(size_t i = 0; i < ITERATIONS; i++) {
    randombytes(m, MIN(i, MAXMBYTES));
    Mitaka_sign_expanded_sk(&s, m, MIN(i, MAXMBYTES), &expanded_sk[0]);
    correct += Mitaka_verify(m, MIN(i, MAXMBYTES), &pk[0], &s);
  }
  printf("  %d/%d correct signatures. (%s).\n\n", correct, ITERATIONS,
    (correct == ITERATIONS)?"ok":"ERROR!");

  correct = 0;
  for(size_t i = 0; i < ITERATIONS; i++) {
    randombytes(m, MIN(i, MAXMBYTES));
    Mitaka_sign_expanded_sk(&s, m, MIN(i, MAXMBYTES), &expanded_sk[1]);
    correct += Mitaka_verify(m, MIN(i, MAXMBYTES), &pk[1], &s);
  }
  printf("  %d/%d correct signatures. (%s).\n\n", correct, ITERATIONS,
    (correct == ITERATIONS)?"ok":"ERROR!");

  correct = 0;
  for(size_t i = 0; i < ITERATIONS; i++) {
    randombytes(m, MIN(i, MAXMBYTES));
    Mitaka_sign_expanded_sk(&s, m, MIN(i, MAXMBYTES), &expanded_sk[0]);
    correct += Mitaka_verify(m, MIN(i, MAXMBYTES), &pk[1], &s);
  }
  printf("  %d/%d correct signatures. (%s).\n\n", correct, ITERATIONS,
    (correct == 0)?"ok":"ERROR!");

  correct = 0;
  for(size_t i = 0; i < ITERATIONS; i++) {
    randombytes(m, MIN(i, MAXMBYTES));
    Mitaka_sign_expanded_sk(&s, m, MIN(i, MAXMBYTES), &expanded_sk[1]);
    correct += Mitaka_verify(m, MIN(i, MAXMBYTES), &pk[0], &s);
  }
  printf("  %d/%d correct signatures. (%s).\n\n", correct, ITERATIONS,
    (correct == 0)?"ok":"ERROR!");


  printf("* Test correctness of Gandalf.\n");

  correct = 0;
  for(size_t i = 0; i < ITERATIONS; i++) {
    randombytes(m, MIN(i, MAXMBYTES));
    party_id = rand() % RING_K;
    Gandalf_sign(&Gandalf_s, m, MIN(i, MAXMBYTES), &pks, sk + party_id, party_id);
    correct += Gandalf_verify(m, MIN(i, MAXMBYTES), &Gandalf_s, &pks);
  }
  printf("  %d/%d correct signatures. (%s).\n\n", correct, ITERATIONS,
	  (correct == ITERATIONS)?"ok":"ERROR!");


  correct = 0;
  for(size_t i = 0; i < ITERATIONS; i++) {
    randombytes(m, MIN(i, MAXMBYTES));
    party_id = rand() % RING_K;
    Gandalf_sign(&Gandalf_s, m, MIN(i, MAXMBYTES), &pks, sk + RING_K, party_id);
    correct += Gandalf_verify(m, MIN(i, MAXMBYTES), &Gandalf_s, &pks);
  }
  printf("  %d/%d correct signatures. (%s).\n\n", correct, ITERATIONS,
    (correct == 0)?"ok":"ERROR!");



  return 0;
}


