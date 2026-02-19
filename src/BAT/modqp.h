#ifndef MODQP_H
#define MODQP_H

#include <stdint.h>

/* ====================================================================== */
/*
 * Computations on polynomials modulo q' = 64513.
 */

/*
 * Compute d = -a*b mod X^n+1 mod q'
 * Coefficients of source values are plain integers (for value b, they must
 * be in the -503109..+503109 range). Coefficients of output values are
 * normalized in -32256..+32256.
 *
 * Array d[] may overlap, partially or totally, with a[]; however, it
 * MUST NOT overlap with b[].
 *
 * Size of tmp[]: n/2 elements (2*n bytes).
 */
void bat_polyqp_mulneg(int16_t *d, const int16_t *a, const int32_t *b,
    unsigned logn, uint32_t *tmp);

#endif

