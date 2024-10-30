#ifndef PACK_UNPACK_H
#define PACK_UNPACK_H

#include "rsig_params.h"
#include "poly.h"

void pack_h(uint8_t *des, const poly *src);
void unpack_h(poly *des, const uint8_t *src);

#endif

