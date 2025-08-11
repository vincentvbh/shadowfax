
#include "sign_keygen.h"
#include "poly.h"
#include "encode_decode.h"
#include "compute_keys.h"
#include "randombytes.h"

#include "ng_fxp.h"
#include "ng_fft.h"
#include "ng_ntru.h"

static void simple_frand(double *r, uint64_t *buf, size_t n) {
    double pow2m64 = pow(2.0, -64);
    randombytes((uint8_t*)buf, n*sizeof(uint64_t));
    for(size_t i=0; i<n; i++) {
      r[i] = ((double)buf[i]) * pow2m64;
    }
}

/*
 * Decode a real vector as an integer vector with odd sum.
 *
 * In other words: solve CVP in the non trivial coset of the D_n lattice.
 *
 * This is done using the decoder described in Conway & Sloane 20.2:
 * round all coefficients to the nearest integer, and if the sum is even,
 * round the worst coefficient (the one farthest from Z) in the other
 * direction.
 *
 * This implementation below is deliberately not constant time (since we
 * are not aiming for a constant-time keygen), but this is
 * straightforward to fix if deemed necessary.
 */
static void decode_odd(int8_t u[N], const fpoly *utilde){

    uint8_t umod2 = 0;
    int8_t ui, wi = 0;
    size_t worst_coeff = 0;
    double maxdiff = -1, uitilde, diff;

    for(size_t i=0; i<N; i++) {
      uitilde = utilde->coeffs[i].v;
      ui      = lrint(uitilde);
      umod2  ^= ui;
      diff    = fabs(uitilde - (double)ui);
      if(diff > maxdiff) {
        worst_coeff = i;
        maxdiff = diff;
      if(uitilde > (double)ui)
        wi = ui + 1;
      else
        wi = ui - 1;
      }
      u[i] = ui;
    }
    if((umod2 & 1) == 0)
      u[worst_coeff] = wi;

}

int keygen_fg(sign_sk *sk){

  double z[N/2], af[N/2], ag[N/2],
        f[N], g[N];
#if 0
  const double alow  = 1/ALPHA + ALPHAEPS,
           ahigh = ALPHA - ALPHAEPS,
#else
  const double alow  = 0.5*(ALPHA + 1/ALPHA) - 0.5*ANTRAG_XI*(ALPHA-1/ALPHA),
           ahigh = 0.5*(ALPHA + 1/ALPHA) + 0.5*ANTRAG_XI*(ALPHA-1/ALPHA),
#endif
  qlow  = ((double)Q)*alow*alow,
           qhigh = ((double)Q)*ahigh*ahigh,
           qlow2 = ((double)Q)/(ALPHA*ALPHA),
           qhigh2= ((double)Q)*ALPHA*ALPHA;

  double r[2*N];
  uint64_t rint[2*N];

  fpoly ft0, ft1;
  poly t0, t1;

  int trials=0, check;

  do {
    trials++;
    simple_frand(r, rint, 2*N);

    for(size_t i = 0; i < N / 2; i++) {
      z[i] = sqrt(qlow + (qhigh - qlow)*r[i]);

      af[i]            = z [i] * cos(M_PI/2*r[i+  N/2]);
      ag[i]            = z [i] * sin(M_PI/2*r[i+  N/2]);
      f [i]            = af[i] * cos(2*M_PI*r[i+2*N/2]);
      f [i+N/2] = af[i] * sin(2*M_PI*r[i+2*N/2]);
      g [i]            = ag[i] * cos(2*M_PI*r[i+3*N/2]);
      g [i+N/2] = ag[i] * sin(2*M_PI*r[i+3*N/2]);
    }
    for(size_t i = 0; i < N; i++) {
      ft0.coeffs[i].v = f[i];
      ft1.coeffs[i].v = g[i];
    }
    fpoly_iFFT(&ft0);
    fpoly_iFFT(&ft1);

    decode_odd(sk->f, &ft0);
    decode_odd(sk->g, &ft1);

    for(size_t i = 0; i < N; i++) {
      t0.coeffs[i] = (int32_t)sk->f[i];
      t1.coeffs[i] = (int32_t)sk->g[i];
    }

    poly_2_fpoly(&ft0, &t0);
    poly_2_fpoly(&ft1, &t1);

    fpoly_FFT(&ft0);
    fpoly_FFT(&ft1);
    check = 0;
    for(size_t i = 0; i < N / 2; i++) {
      double zi =
        fpr_sqr(ft0.coeffs[i]).v +
        fpr_sqr(ft0.coeffs[i+N/2]).v +
        fpr_sqr(ft1.coeffs[i]).v +
        fpr_sqr(ft1.coeffs[i+N/2]).v;
      if(zi < qlow2 || zi > qhigh2) {
        check = 1;
        break;
      }
    }

  } while(check);
    return trials;

}

int sign_keygen(sign_sk *sk, sign_pk *pk){

    int trials = 0;
    uint32_t tmp_uint32[8 * N];

    while(1) {

        trials += keygen_fg(sk);

        if (!compute_public(pk->h, sk->f, sk->g))
            continue;

        if (solve_NTRU(&SOLVE_Falcon_512, LOG_N, sk->f, sk->g, sk->F, sk->G, tmp_uint32) != SOLVE_OK)
            continue;

        break;
    }

    return trials;

}

int sign_keygen_expanded_sk(sign_expanded_sk *expanded_sk, sign_pk *pk){

    int trials = 0;
    sign_sk sk;

    trials = sign_keygen(&sk, pk);

    expand_sign_sk(expanded_sk, &sk);

    return trials;

}

