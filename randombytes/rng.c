/*
 * PRNG and interface to the system RNG.
 *
 * ==========================(LICENSE BEGIN)============================
 *
 * Copyright (c) 2017-2019  Falcon Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * ===========================(LICENSE END)=============================
 *
 * @author   Thomas Pornin <thomas.pornin@nccgroup.com>
 */

#include "rng.h"

#include <memory.h>

/*
 * Initialize the PRNG from the provided seed and an extra 64-bit integer.
 * The seed length MUST NOT exceed 48 bytes.
 */
void
prng_init(prng *p, const void *seed, size_t seed_len, uint64_t label)
{
	blake2s_expand(p->key.d, sizeof p->key.d, seed, seed_len, label);
    p->ptr = sizeof p->buf;
    p->ctr = 0;
}

/* see rng.h */
int
prng_get_bytes(prng *p, void *dst, size_t len)
{
	blake2s_expand(dst, len, p->key.d, sizeof p->key.d, p->ctr++);
	return (int)len;
}


