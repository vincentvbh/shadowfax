#ifndef NG_NTRU_H
#define NG_NTRU_H

#include <stdint.h>

/* ==================================================================== */

/*
 * Lengths of values (big integers) in 31-bit limbs:
 *   max_bl_small[d]      max length of coeffs of (f,g) at depth d
 *   max_bl_small[logn]   max length of Res(f,phi) and Res(g,phi)
 *   max_bl_large[d]      max length of coeffs of unreduced (F,G) at depth d
 *   word_win[d]          number of top limbs to consider in (f,g) (Babai's NP)
 * Rules:
 *   max_bl_small[0] = 1
 *   max_bl_large[d] >= max_bl_small[d + 1]
 *   1 <= word_win[d] <= max_bl_small[d]
 * Additional rules to use the optimized depth0 function:
 *   max_bl_large[0] = 1
 *   max_bl_small[1] = 1
 *
 * q                target integer (f*G - g*F = q)
 * min_logn         minimum logn supported
 * max_logn         maximum logn supported
 * reduce_bits      assumed reduction per iteration (in bits)
 * coeff_FG_limit   maximum allowed value for coefficients of F and G
 * min_save_fg      minimum depth at which (f,g) can be saved temporarily
 */
typedef struct {
    uint32_t q;
    unsigned min_logn, max_logn;
    uint16_t max_bl_small[11];
    uint16_t max_bl_large[10];
    uint16_t word_win[10];
    uint32_t reduce_bits;
    uint8_t coeff_FG_limit[11];
    uint16_t min_save_fg[11];
} ntru_profile;

extern const ntru_profile SOLVE_BAT_128_256;
extern const ntru_profile SOLVE_BAT_257_512;
extern const ntru_profile SOLVE_BAT_769_1024;
extern const ntru_profile SOLVE_Falcon_512;

/* Error code: no error (so far) */
#define SOLVE_OK           0

/* Error code: GCD(Res(f,X^n+1), Res(g,X^n+1)) != 1 */
#define SOLVE_ERR_GCD      -1

/* Error code: reduction error (NTRU equation no longer fulfilled) */
#define SOLVE_ERR_REDUCE   -2

/* Error code: output (F,G) coefficients are off-limits */
#define SOLVE_ERR_LIMIT    -3

/*
 * Solve the NTRU equation for the provided (f,g).
 * The (F,G) solution (if found) is returned at the start of the tmp[]
 * array, as two consecutive int8_t[] values. Returned value is
 * SOLVE_OK (0) on success, a negative error code on failure.
 *
 * Note: if f is not invertible modulo X^n+1 and modulo p = 2147473409,
 * then an error (SOLVE_ERR_GCD) is reported. This test is not necessary
 * for the computation itself, but fulfilling it implies that G will
 * be recoverable later on from f, g and F. Only a very small proportion
 * of possible polynomials f are not invertible modulo X^n+1 and p.
 *
 * RAM USAGE: 6*n words
 */
int solve_NTRU(const ntru_profile *prof, unsigned logn,
    const int8_t *restrict f, const int8_t *restrict g,
    int8_t *restrict F, int8_t *restrict G, uint32_t *tmp);

/*
 * Recompute G from f, g and F (using the NTRU equation f*G - g*F = q).
 * This may fail if f is not invertible modulo X^n+1 and modulo
 * p = 2147473409. However, the rest of this key pair generator code takes
 * care never to generate keys with such polynomials f.
 *
 * G is returned as the first n bytes of tmp.
 *
 * Returned value is 1 on success, 0 on error. An error is reported if
 * f is not invertible, or if any of the reconstructed G coefficients is
 * not in the [-lim..+lim] range.
 *
 * RAM USAGE: 3*n words
 */
int recover_G(unsigned logn, int32_t q, uint32_t lim,
    const int8_t *restrict f, const int8_t *restrict g,
    const int8_t *restrict F, uint32_t *restrict tmp);

/* ==================================================================== */

#endif

