
#include "ng_fxp.h"

/* see ng_fxp.h */
uint64_t
inner_fxr_div(uint64_t x, uint64_t y)
{
	/*
	 * Get absolute values and signs. From now on, we can suppose
	 * that x and y fit on 63 bits (we ignore edge conditions).
	 */
	uint64_t sx = x >> 63;
	x = (x ^ -sx) + sx;
	uint64_t sy = y >> 63;
	y = (y ^ -sy) + sy;

	/*
	 * Do a bit by bit division, assuming that the quotient fits.
	 * The numerator starts at x*2^31, and is shifted one bit a time.
	 */
	uint64_t q = 0;
	uint64_t num = x >> 31;
	for (int i = 63; i >= 33; i --) {
		uint64_t b = 1 - ((num - y) >> 63);
		q |= b << i;
		num -= y & -b;
		num <<= 1;
		num |= (x >> (i - 33)) & 1;
	}
	for (int i = 32; i >= 0; i --) {
		uint64_t b = 1 - ((num - y) >> 63);
		q |= b << i;
		num -= y & -b;
		num <<= 1;
	}

	/*
	 * Rounding: if the remainder is at least y/2 (scaled), we add
	 * 2^(-32) to the quotient.
	 */
	uint64_t b = 1 - ((num - y) >> 63);
	q += b;

	/*
	 * Sign management: if the original x and y had different signs,
	 * then we must negate the quotient.
	 */
	sx ^= sy;
	q = (q ^ -sx) + sx;

	return q;
}

