
#include "randombytes.h"
#include "fft.h"
#include "poly.h"
#include "mitaka_sampler.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

uint64_t CDT[TABLE_SIZE] = {8562458705743934607LLU,
                           14988938141546119862LLU,
                           17705984313312429518LLU,
                           18353082494776078532LLU,
                           18439897061947435901LLU,
                           18446457975170112665LLU,
                           18446737284374178633LLU,
                           18446743982533372247LLU,
                           18446744073018029834LLU,
                           18446744073706592852LLU,
                           18446744073709544480LLU,
                           18446744073709551607LLU,
                           18446744073709551615LLU};

/* Constant-time macros */
#define LSBMASK(c)      (-((c)&1))
#define CMUX(x,y,c)     (((x)&(LSBMASK(c)))^((y)&(~LSBMASK(c))))
#define CFLIP(x,c)      CMUX(x,-(x),c)
#define CZERO64(x)      ((~(x)&((x)-1))>>63)

/* Store a double-precision centered normal vector in vec.
 * Computation using Box-Muller. N is assumed to be even.
 * Standard deviation is N/2, so that the output vector has
 * the same distribution as the FFT of a standard normal vector.
 */
void normaldist(fpoly *vec)
{
    uint64_t u[N/2], v[N/2], e[N];
    double uf[N/2], vf[N/2];
    int geom[N/2];

    randombytes((uint8_t*)u, sizeof u);
    randombytes((uint8_t*)v, sizeof v);
    randombytes((uint8_t*)e, sizeof e);

    /*
    for(int i=0; i < N/2; ++i){
      u[i] = get64();
      v[i] = get64();
      e[i] = get64();
      e[i+N/2] = get64();
    }
    */

    for(int i=0; i < N/2; i++) {
        uf[i] = 2*M_PI*(double)(u[i] & 0x1FFFFFFFFFFFFFul) * pow(2,-53);
        vf[i] = 0.5 + (double)(v[i] & 0x1FFFFFFFFFFFFFul) * pow(2,-54);

        geom[i] = CMUX(63 + ffsll(e[2*i+1]), ffsll(e[2*i]) - 1,
            CZERO64(e[2*i]));

        vf[i] = sqrt(N*(M_LN2*geom[i]-log(vf[i])));
    }

    for(int i=0; i < N/2; i++) {
        vec->coeffs[2*i].v   = vf[i] * cos(uf[i]);
        vec->coeffs[2*i+1].v = vf[i] * sin(uf[i]);
    }
}

int32_t base_sampler(){
    uint64_t r = get64();
    int32_t res = 0;
    for(size_t i = 0; i < TABLE_SIZE; ++i)
        res += (r >= CDT[i]);
    return res;
}

int32_t samplerZ(double u){
    int32_t z0, b, z, uf;
    double x, p, r;
    uint8_t entropy;
    uf = floor(u);
    while (1){
        entropy = get8();
        for(int i=0; i < 8; ++i){
            z0 = base_sampler();
            b = (entropy >> i) & 1;
            z = (2 * b - 1) * z0 + b + uf; // add floor u here because we subtract u at the next step
            x = ((double)(z0 * z0)-((double)(z - u) * (z - u))) / (2 * R_SQUARE);
            p = exp(x);

            /*
            r = (double)get64()/(1LLU<<63);
            r /= 2;
            */
            r = (double)(get64() & 0x1FFFFFFFFFFFFFul) * pow(2,-53);
            if (r < p)
                return z;
        }
    }
}

void sample_discrete_gauss(poly *des, const fpoly *src){
    for(size_t i = 0; i < N; i++) {
        des->coeffs[i] = samplerZ(src->coeffs[i].v);
    }
}

// since c1 = 0, we don't need to pass its value
// beta's and sigma's are all in FFT domain
void sampler(poly *v0_ptr, poly *v1_ptr, const sign_expanded_sk *sk, const poly c2){

    fpoly d, temp;
    fpoly FFTc2;
    fpoly FFTy1, FFTy2;
    fpoly FFTv0, FFTv1;

    poly v0, v1;
    poly acc0, acc1;
    poly _d;
    poly nc2;

    nc2 = c2;
    poly_neg(&nc2, &c2);

    // offline
    // we sample in FFT domain
    normaldist(&FFTy1);
    normaldist(&FFTy2);
    fpoly_pointwise_mul(&FFTy1, &(sk->sigma1));
    fpoly_pointwise_mul(&FFTy2, &(sk->sigma2));

    // online
    // first nearest plane
    poly_2_fpoly(&FFTc2, &nc2);
    fpoly_FFT(&FFTc2);
    d = FFTc2;
    fpoly_pointwise_mul(&d, &sk->beta21); //d2 fft form
    fpoly_sub(&d, &FFTy2);
    fpoly_iFFT(&d); // x2 = d2 - y2

    sample_discrete_gauss(&_d, &d);

    // second nearest plane
    // figure out which modulus can be used here
    poly_mul_big(&v0, &_d, &sk->b20);
    poly_mul_big(&v1, &_d, &sk->b21);

    poly_2_fpoly(&FFTv0, &v0);
    poly_2_fpoly(&FFTv1, &v1);

    fpoly_FFT(&FFTv0);
    fpoly_FFT(&FFTv1);
    fpoly_sub(&FFTc2, &FFTv1);

    temp = FFTv0;
    d    = FFTc2;
    fpoly_pointwise_mul(&temp, &sk->beta10);
    fpoly_pointwise_mul(&d, &sk->beta11);
    fpoly_sub(&d, &temp); //d1 fft form
    fpoly_sub(&d, &FFTy1);
    fpoly_iFFT(&d); // x1 = d1 - y1

    sample_discrete_gauss(&_d, &d);

    poly_mul(&acc0, &_d, &sk->b10);
    poly_mul(&acc1, &_d, &sk->b11);

    poly_add(&v0, &v0, &acc0);
    poly_add(&v1, &v1, &acc1);

    poly_neg(&v0, &v0);
    poly_sub(&v1, &v1, &nc2);

    poly_freeze(v0_ptr, &v0);
    poly_freeze(v1_ptr, &v1);

}

