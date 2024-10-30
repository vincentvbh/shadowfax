#ifndef RSIG_PARAMS_H
#define RSIG_PARAMS_H

#include "rsig_api.h"

#define N 512
#define LOG_N 9
#define Q 12289
// #define RING_K 2
#define ALPHA 1.15
#define R_SQUARE 1.6329
// #define R_SQUARE 1.7424
#define GANDALF_BOUND_SQUARE_FLOOR (60669689)

// #define SALT_BYTES 24
#define ZQPOLY_BYTES (2 * N)
#define FPOLY_BYTES (8 * N)

// #define SIGN_PUBLICKEY_BYTES (ZQPOLY_BYTES)
// #define SIGN_SECRETKEY_BYTES (4 * N)
// #define COMPRESSED_SIGN_SIGNATURE_BYTES (626)
// #define SIGN_SIGNATURE_BYTES (COMPRESSED_SIGN_SIGNATURE_BYTES + SALT_BYTES)

// #define rsig_PUBLICKEY_BYTES (RING_K * SIGN_PUBLICKEY_BYTES)
// #define rsig_SIGNATURE_BYTES (RING_K * COMPRESSED_SIGN_SIGNATURE_BYTES + SALT_BYTES)

#define ANTRAG_XI (1/3.)

/* sigma^2 = r^2 * alpha^2 * q */
/* gamma^2 = slack^2 * sigma^2 * 2d */
/* slack = 1.042 */
#define ANTRAG_SLACK 1.042
#define SIGMA_SQUARE (R_SQUARE * ALPHA * ALPHA * Q)
#define MITAKA_BOUND_SQUARE (ANTRAG_SLACK * ANTRAG_SLACK * SIGMA_SQUARE * D * 2)
#define MITAKA_BOUND_SQUARE_FLOOR (31484404)


#endif

