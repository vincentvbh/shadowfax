
#include "hash.h"
#include "poly.h"

void hash_to_poly(poly *out, shake128incctx *state){

  unsigned int ctr = 0;
  uint16_t val;
  uint8_t buf[SHAKE128_RATE];
  const uint16_t modulus = Q;
  const size_t dim = N;
  const ZArithData data = {Q, BarrettFactor, NTTFinalFactor, 1, 1, 1, 1};
  int i;

  ctr = 0;
  while(ctr < dim)
  {
    shake128_inc_squeeze(buf, SHAKE128_RATE, state);
    for(i=0;i < SHAKE128_RATE && ctr < dim; i += 2)
    {
      val = (buf[i] | ((uint16_t) buf[i+1] << 8));
      if(val < 5 * modulus)
      {
        out->coeffs[ctr] = freeze_generic((int32_t)val, data);
        ctr++;
      }
    }
  }


}

