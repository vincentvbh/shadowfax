#ifndef NG_FFT_H
#define NG_FFT_H

#include "ng_fxp.h"

/*
 * In FFT representation, we keep only half of the coefficients, because
 * all our vectors are real in non-FFT representation; thus, the FFT
 * representation is redundant. For 0 <= k < n/2, f[k] contains the
 * real part of FFT coefficient k, and f[k + n/2] contains the imaginary
 * part of FFT coefficient k.
 */

/*
 * Convert a (real) vector to its FFT representation.
 */
void vect_FFT(unsigned logn, fxr *f);

/*
 * Convert back from FFT representation into a real vector.
 */
void vect_iFFT(unsigned logn, fxr *f);

/*
 * Set a vector d to the value of the small polynomial f.
 */
void vect_set(unsigned logn, fxr *d, const int8_t *f);

/*
 * Add vector b to vector a. This works in both real and FFT representations.
 * Vectors a and b MUST NOT overlap.
 */
void vect_add(unsigned logn, fxr *restrict a, const fxr *restrict b);

/*
 * Multiply vector a by the real constant c. This works in both real
 * and FFT representations.
 */
void vect_mul_realconst(unsigned logn, fxr *a, fxr c);

/*
 * Multiply a vector by 2^e. This works in both real and FFT representations.
 */
void vect_mul2e(unsigned logn, fxr *a, unsigned e);

/*
 * Multiply vector a by vector b. The vectors must be in FFT representation.
 * Vectors a and b MUST NOT overlap.
 */
void vect_mul_fft(unsigned logn, fxr *restrict a, const fxr *restrict b);

/*
 * Multiply vector a by the adjoint vector of b. The vectors must be in FFT representation.
 * Vectors a and b MUST NOT overlap.
 */
void vect_muladj_fft(unsigned logn, fxr *restrict a, const fxr *restrict b);

/*
 * Convert a vector into its adjoint (in FFT representation).
 */
void vect_adj_fft(unsigned logn, fxr *a);

/*
 * Multiply vector a by auto-adjoint vector b. The vectors must be in FFT
 * representation. Since the FFT representation of an auto-adjoint vector
 * contains only real number, the second half of b contains only zeros and
 * is not accessed by this function. Vectors a and b MUST NOT overlap.
 */
void vect_mul_autoadj_fft(unsigned logn,
    fxr *restrict a, const fxr *restrict b);

/*
 * Divide vector a by auto-adjoint vector b. The vectors must be in FFT
 * representation. Since the FFT representation of an auto-adjoint vector
 * contains only real number, the second half of b contains only zeros and
 * is not accessed by this function. Vectors a and b MUST NOT overlap.
 */
void vect_div_autoadj_fft(unsigned logn,
    fxr *restrict a, const fxr *restrict b);

void vect_div_fft(unsigned logn,
    fxr *restrict a, const fxr *restrict b);

/*
 * Compute d = a*adj(a) + b*adj(b). Polynomials are in FFT representation.
 * Since d is auto-adjoint, only its first half is set; the second half
 * is _implicitly_ zero (this function does not access the second half of d).
 * Vectors a, b and d MUST NOT overlap.
 */
void vect_norm_fft(unsigned logn, fxr *restrict d,
    const fxr *restrict a, const fxr *restrict b);

/*
 * Compute d = (2^e)/(a*adj(a) + b*adj(b)). Polynomials are in FFT
 * representation. Since d is auto-adjoint, only its first half is set; the
 * second half is _implicitly_ zero (this function does not access the
 * second half of d). Vectors a, b and d MUST NOT overlap.
 */
void vect_invnorm_fft(unsigned logn, fxr *restrict d,
    const fxr *restrict a, const fxr *restrict b, unsigned e);

#endif

