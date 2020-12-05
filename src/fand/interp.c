#include "interp.h"

#include <math.h>

float inverse_lerp(unsigned long a, unsigned long b, unsigned long x) {
    return (float)(x - a) / (float)(b - a);
}

unsigned long lerp(unsigned long a, unsigned long b, float x) {
    return round(a + (b - a) * x);
}