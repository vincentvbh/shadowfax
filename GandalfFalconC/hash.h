#ifndef HASH_H
#define HASH_H

#include "fips202.h"
#include "poly.h"

#include <stdint.h>
#include <stddef.h>

// We only accept elements smaller than ANTRAG_Q here.
void hash_to_poly(poly *out, shake128incctx *state);

#endif

