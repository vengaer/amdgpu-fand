#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <math.h>
#include <stdint.h>


/*
 * Linear interpolation
 */
static inline uint8_t lerp_uint8(uint8_t a, uint8_t b, double x) {
    return (uint8_t)(round(a + (b - a) * x));
}

/*
 * "Inverse" linear interpolation
 * Find the percentage of the distance a-b that
 * a-x covers (e.g. if x is halfway between a and b, 
 * the distance percentage is 50).
 *
 * |--------------#------|
 * a              x      b
 */
static inline uint8_t inverse_lerp_uint8(uint8_t a, uint8_t b, uint8_t x) {
    return (uint8_t)round(100.0 * (double)(x - a) / (double)(b - a));
}

#endif
