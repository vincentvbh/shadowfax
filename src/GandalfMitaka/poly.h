#ifndef POLY_H
#define POLY_H

#include "rsig_api.h"
#include "rsig_params.h"
#include "fft.h"

#include <stdint.h>
#include <stdbool.h>

// ========
// fpoly

void print_fpoly(const fpoly* p);
void fpoly_FFT(fpoly* p);
void fpoly_iFFT(fpoly* p);
void fpoly_pointwise_mul(fpoly* p1, const fpoly* p2);
void FFT_adj(fpoly* p);
void FFT_mul_adj(fpoly* p1, const fpoly* p2);
void FFT_mul_selfadj(fpoly *p1);
void fpoly_div_FFT(fpoly* p1, const fpoly* p2);

void naive_mul(fpoly* c, const fpoly* a, const fpoly* b);
void fpoly_add(fpoly* p1, const fpoly* p2);
void fpoly_sub(fpoly* p1, const fpoly* p2);

void test_integral(const fpoly *p);
void ensure_integral(fpoly *des, const fpoly *src);

// ========
// poly

typedef struct {
    int32_t mod;
    int32_t modBarrettFactor;
    int32_t modNTTFinalFactor;
    int32_t Rmod;
    int32_t R2mod;
    int32_t modMontgomeryFactor;
    int32_t modMontgomeryNTTFinalFactor;
} ZArithData;

#define NTT_N 512
#define LOGNTT_N 9

void print_poly(poly *src);

void poly_add(poly *des, const poly *src1, const poly *src2);
void poly_sub(poly *des, const poly *src1, const poly *src2);
void poly_neg(poly *des, const poly *src);

// round(2^32 / Q)
#define BarrettFactor 349497
// BarrettBound is found by exponential-search
// log_2(BarrettBound) < 20
#define BarrettBound 1616003
// NTT_N^(-1) mod^+- Q
#define NTTFinalFactor (-24)

// 2^32 mod^+- Q
#define RmodQ (-1337)
// 2^64 mod^+- Q
#define R2modQ (5664)
// Q^(-1) mod^+- 2^32
#define MontgomeryFactor (150982657)
// 2^64 * (NTT_N)^(-1) mod^+- Q
#define MontgomeryNTTFinalFactor (-757)

int32_t barrett_generic(int64_t a, ZArithData data);
int32_t freeze_generic(int32_t a, ZArithData data);
int32_t mq_div(int32_t x, int32_t y);

void poly_freeze(poly *des, const poly *src);
void poly_ufreeze(poly *des, const poly *src);

void poly_NTT_generic(poly *src, int32_t *twiddle_table, ZArithData data);
void poly_iNTT_generic(poly *src, int32_t *twiddle_table, ZArithData data);
void poly_point_mul_generic(poly *des, const poly *src1, const poly *src2, ZArithData data);

void poly_mul(poly *des, const poly *src1, const poly *src2);
bool poly_test_inv(const poly src);
bool poly_div(poly *des, const poly *src1, const poly *src2);

void poly_mul_big(poly *des, const poly *src1, const poly *src2);

// ========
// conversion

void fpoly_2_poly(poly *des, const fpoly *src);
void poly_2_fpoly(fpoly *des, const poly *src);

#endif

