
#include "poly.h"
#include "compute_keys.h"
#include "pack_unpack.h"

#include <stddef.h>
#include <math.h>

bool compute_public(uint8_t *h, const int8_t *f, const int8_t *g){

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

void compute_GSO(sign_expanded_sk* sk){

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

void compute_sigma(sign_expanded_sk* sk){

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
void compute_beta_hat(sign_expanded_sk* sk){

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






