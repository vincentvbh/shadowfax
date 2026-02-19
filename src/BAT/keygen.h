#ifndef KEYGEN_H
#define KEYGEN_H

#include <stdint.h>
#include <stddef.h>

/* ====================================================================== */

/*
 * Key pair generation, first step: given a seed, candidate polynomials
 * f and g are generated. The following properties are checked:
 *  - All coefficients of f and g are within the expected bounds.
 *  - Res(f, x^n+1) == 1 mod 2.
 *  - Res(g, x^n+1) == 1 mod 2.
 *  - The (f,g) vector has an acceptable norm, both in normal and in
 *    orthogonalized representations.
 *  - f is invertible modulo x^n+1 modulo q.
 * If any of these properties is not met, then a failure is reported
 * (returned value is 0) and the contents of f[] and g[] are indeterminate.
 * Otherwise, success (1) is returned.
 *
 * If h != NULL, then the public key h = g/f mod x^n+1 mod q is returned
 * in that array. Note that h is always internally computed, regardless
 * of whether h == NULL or not.
 *
 * Size of tmp[]: 6*n elements (24*n bytes).
 * tmp[] MUST be 64-bit aligned.
 *
 * The seed length MUST NOT exceed 48 bytes.
 */
int bat_keygen_make_fg(int8_t *f, int8_t *g, uint16_t *h,
    uint32_t q, unsigned logn,
    const void *seed, size_t seed_len, uint32_t *tmp);

/*
 * Given polynomials f and g, solve the NTRU equation for F and G. This
 * may fail if there is no solution, or if some intermediate value exceeds
 * an internal heuristic threshold. Returned value is 1 on success, 0
 * on failure. On failure, contents of F and G are indeterminate.
 *
 * Size of tmp[]: 6*n elements (24*n bytes).
 * tmp[] MUST be 64-bit aligned.
 */
int bat_keygen_solve_FG(int8_t *F, int8_t *G,
    const int8_t *f, const int8_t *g,
    uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * Given polynomials f, g and F, rebuild the polynomial G that completes
 * the NTRU equation g*F - f*G = q. Returned value is 1 on success, 0 on
 * failure. A failure is reported if the rebuilt solution has
 * coefficients outside of the expected maximum range, or f is not
 * invertible modulo x^n+1 modulo q. This function does NOT fully verify
 * that f, g, F, G is a solution to the NTRU equation.
 *
 * Size of tmp[]: n elements (4*n bytes).
 */
int bat_keygen_rebuild_G(int8_t *G,
    const int8_t *f, const int8_t *g, const int8_t *F,
    uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * Verify that the given f, g, F, G fulfill the NTRU equation g*F - f*G = q.
 * Returned value is 1 on success, 0 on error.
 *
 * This function may be called when decoding a private key of unsure
 * provenance. It is implicitly called by bat_keygen_solve_FG().
 *
 * Size of tmp[]: 4*n elements (16*n bytes).
 */
int bat_keygen_verify_FG(
    const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
    uint32_t q, unsigned logn, uint32_t *tmp);

/*
 * Compute the w vector. Returned value is 1 on success, 0 on error. An
 * error is reported if the w vector has coefficients that do not fit
 * in signed 16-bit integer, or if the norm of (gamma*F_d, G_d) exceeds
 * the prescribed limit.
 *
 * Size of tmp[]: 6*n elements (24*n bytes).
 * tmp[] MUST be 64-bit aligned.
 */
int bat_keygen_compute_w(int32_t *w,
    const int8_t *f, const int8_t *g, const int8_t *F, const int8_t *G,
    uint32_t q, unsigned logn, uint32_t *tmp);

#endif








