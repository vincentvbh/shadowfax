
#include "pack_unpack.h"
#include "encode_decode.h"
#include "rsig_params.h"

#include <stddef.h>

void pack_h(uint8_t *des, const poly *src){
    modq_encode32(des, SIGN_PUBLICKEY_BYTES, (const uint32_t*)src->coeffs, LOG_N);
}

void unpack_h(poly *des, const uint8_t *src){
    modq_decode32((uint32_t*)des->coeffs, LOG_N, src, SIGN_PUBLICKEY_BYTES);
}


