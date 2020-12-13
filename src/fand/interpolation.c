#include "interpolation.h"

#include <math.h>

float lerp_inverse(unsigned long a, unsigned long b, unsigned long x) {
    return (float)(x - a) / (float)(b - a);
}

unsigned long lerp(unsigned long a, unsigned long b, float x) {
    return round(a + (b - a) * x);
}
