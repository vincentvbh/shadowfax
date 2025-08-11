
#include "mitaka_keygen.h"
#include "poly.h"
#include "encode_decode.h"
#include "randombytes.h"
#include "pack_unpack.h"

#include "ng_fxp.h"
#include "ng_fft.h"
#include "ng_ntru.h"

#include <math.h>

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


static bool compute_public(uint8_t *h, const int8_t *f, const int8_t *g){

    poly src1, src2;
    poly res;

    for(size_t i = 0; i < N; i++){
        src1.coeffs[i] = f[i];
        src2.coeffs[i] = g[i];
    }

    if(poly_div(&res, &src2, &src1) == 0){
        return 0;
    }

    poly_ufreeze(&res, &res);
    pack_h(h, &res);

    return 1;

}

static void compute_GSO(sign_expanded_sk* sk){

    fpoly temp1, temp2, temp3;
    fpoly FFTb10, FFTb11;

    poly_2_fpoly(&sk->GSO_b10, &sk->b10);
    poly_2_fpoly(&sk->GSO_b11, &sk->b11);
    poly_2_fpoly(&sk->GSO_b20, &sk->b20);
    poly_2_fpoly(&sk->GSO_b21, &sk->b21);

    fpoly_FFT(&sk->GSO_b10);
    fpoly_FFT(&sk->GSO_b11);
    fpoly_FFT(&sk->GSO_b20);
    fpoly_FFT(&sk->GSO_b21);

    FFTb10 = sk->GSO_b10;
    FFTb11 = sk->GSO_b11;

    temp1 = sk->GSO_b20;
    temp2 = sk->GSO_b21;

    FFT_mul_adj(&temp1, &FFTb10); FFT_mul_adj(&temp2, &FFTb11);
    fpoly_add(&temp1, &temp2); // temp1 = <b1, b2>

    temp2 = FFTb10;
    temp3 = FFTb11;

    FFT_mul_selfadj(&temp2); FFT_mul_selfadj(&temp3);
    fpoly_add(&temp2, &temp3); // temp2 = <b1, b1>

    fpoly_div_FFT(&temp1, &temp2);

    // temp2 = temp1 = <b1, b2>/<b1, b1>
    temp2 = temp1;

    fpoly_pointwise_mul(&temp1, &(sk->GSO_b10));
    fpoly_pointwise_mul(&temp2, &(sk->GSO_b11)); // [temp1,temp2] = (<b1, b2>/<b1, b1>) * ~b1

    fpoly_sub(&(sk->GSO_b20), &temp1); fpoly_sub(&(sk->GSO_b21), &temp2);

}

static void compute_sigma(sign_expanded_sk* sk){

    fpoly temp1, temp2, poly_r_square;

    for(int i=0; i < N/2; ++i) {
        (sk->sigma1).coeffs[i].v = SIGMA_SQUARE;
        (sk->sigma2).coeffs[i].v = SIGMA_SQUARE;
        poly_r_square.coeffs[i].v = R_SQUARE;
    }
    for(int i=N/2; i < N; ++i) {
        poly_r_square.coeffs[i].v = 0;
        (sk->sigma1).coeffs[i].v = 0;
        (sk->sigma2).coeffs[i].v = 0;
    }

    temp1 = sk->GSO_b10;
    temp2 = sk->GSO_b11;

    FFT_mul_selfadj(&temp1); FFT_mul_selfadj(&temp2);
    fpoly_add(&temp1, &temp2);

    fpoly_div_FFT(&(sk->sigma1), &temp1);
    fpoly_sub(&(sk->sigma1), &poly_r_square);

    for(int i=0; i < N; ++i){
        (sk->sigma1).coeffs[i].v = sqrt((sk->sigma1).coeffs[i].v);
    }

    temp1 = sk->GSO_b20;
    temp2 = sk->GSO_b21;

    FFT_mul_selfadj(&temp1); FFT_mul_selfadj(&temp2);
    fpoly_add(&temp1, &temp2);
    fpoly_div_FFT(&(sk->sigma2), &temp1);
    fpoly_sub(&(sk->sigma2), &poly_r_square);

    for(int i=0; i < N; ++i){
        (sk->sigma2).coeffs[i].v = sqrt((sk->sigma2).coeffs[i].v);
    }

}

// beta10, beta11, beta20, beta21 are in FFT domain.
static void compute_beta_hat(sign_expanded_sk* sk){

    fpoly temp1, temp2;
    // beta1
    temp1 = sk->GSO_b10;
    temp2 = sk->GSO_b11;

    FFT_mul_selfadj(&temp1); FFT_mul_selfadj(&temp2);
    fpoly_add(&temp1, &temp2);

    sk->beta10 = sk->GSO_b10;
    sk->beta11 = sk->GSO_b11;
    FFT_adj(&(sk->beta10)); FFT_adj(&(sk->beta11));
    fpoly_div_FFT(&(sk->beta10), &temp1); fpoly_div_FFT(&(sk->beta11), &temp1);

    // beta2
    temp1 = sk->GSO_b20;
    temp2 = sk->GSO_b21;

    FFT_mul_selfadj(&temp1); FFT_mul_selfadj(&temp2);
    fpoly_add(&temp1, &temp2);

    sk->beta20 = sk->GSO_b20;
    sk->beta21 = sk->GSO_b21;
    FFT_adj(&(sk->beta20)); FFT_adj(&(sk->beta21));
    fpoly_div_FFT(&(sk->beta20), &temp1); fpoly_div_FFT(&(sk->beta21), &temp1);

}

void expand_sign_sk(sign_expanded_sk *expanded_sk, const sign_sk *sk) {

    for(int i=0; i<N; i++) {
        expanded_sk->f[i] = sk->f[i];
        expanded_sk->g[i] = sk->g[i];
        expanded_sk->F[i] = sk->F[i];
        expanded_sk->G[i] = sk->G[i];
    }

    for(int i=0; i<N; i++) {
        expanded_sk->b10.coeffs[i] = (int32_t)expanded_sk->f[i];
        expanded_sk->b11.coeffs[i] = (int32_t)expanded_sk->g[i];
        expanded_sk->b20.coeffs[i] = (int32_t)expanded_sk->F[i];
        expanded_sk->b21.coeffs[i] = (int32_t)expanded_sk->G[i];
    }

    compute_GSO(expanded_sk);
    compute_sigma(expanded_sk);
    compute_beta_hat(expanded_sk);

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

