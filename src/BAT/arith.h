#ifndef ARITH_H
#define ARITH_H

#include <stdint.h>

/*
 * Explicit reduction and conversion to Montgomery representation modulo
 * 257. This works for inputs x in range 0..4278190336.
 */
static inline uint32_t
m257_tomonty(uint32_t x)
{
    x *= 16711935;
    x = (x >> 16) * 257;
    return (x >> 16) + 1;
}

/*
 * Explicit reduction and conversion to Montgomery representation modulo
 * 769. This works for inputs x in range 0..4244636416.
 */
static inline uint32_t
m769_tomonty(uint32_t x)
{
    x *= 452395775;
    x = (x >> 16) * 769;
    x = (x >> 16) + 1;
    x *= 2016233021;
    x = (x >> 16) * 769;
    return (x >> 16) + 1;
}


#endif

