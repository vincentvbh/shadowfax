#ifndef RNG_H
#define RNG_H

#include "blake2.h"

#include <stdint.h>
#include <stddef.h>

/*
 *
 * A PRNG based on ChaCha20 is implemented; it is seeded from a SHAKE256
 * context (flipped) and is used for bulk pseudorandom generation.
 * A system-dependent seed generator is also provided.
 */

/*
 * Structure for a PRNG. This includes a large buffer so that values
 * get generated in advance. The 'state' is used to keep the current
 * PRNG algorithm state (contents depend on the selected algorithm).
 *
 * The unions with 'dummy_u64' are there to ensure proper alignment for
 * 64-bit direct access.
 */
typedef struct {
    union {
        uint8_t d[128]; /* MUST be 512, exactly */
        uint64_t dummy_u64;
    } buf;
    union {
        uint8_t d[32];
        uint64_t dummy_u64;
    } key;
    size_t ptr;
    size_t ctr;
    int type;
} prng;

/*
 * Initialize the PRNG from the provided seed and an extra 64-bit integer.
 * The seed length MUST NOT exceed 48 bytes.
 */
void prng_init(prng *p, const void *seed, size_t seed_len, uint64_t label);

/*
 * Get some bytes from a PRNG.
 */
int prng_get_bytes(prng *p, void *dst, size_t len);

/*
 * Get a 64-bit random value from a PRNG.
 */
static inline uint64_t
prng_get_u64(prng *p)
{

    uint64_t x;
    const uint8_t *ptr;

    if (p->ptr >= (sizeof p->buf) - 9) {
        blake2s_expand(p->buf.d, sizeof p->buf.d, p->key.d, sizeof p->key.d, p->ctr++);
        p->ptr = 0;
    }
    ptr = p->buf.d + p->ptr;
    p->ptr += 8;
    // little-endian only
    x = *((uint64_t*)ptr);
    return x;
}

/*
 * Get an 8-bit random value from a PRNG.
 */
static inline unsigned
prng_get_u8(prng *p)
{
    unsigned v;

    if (p->ptr == sizeof p->buf.d) {
        blake2s_expand(p->buf.d, sizeof p->buf.d, p->key.d, sizeof p->key.d, p->ctr++);
        p->ptr = 0;
    }
    v = p->buf.d[p->ptr ++];
    return v;
}

#endif

