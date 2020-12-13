#include "interpolation.h"
#include "interpolation_test.h"
#include "test.h"

#include <math.h>

#define EPSILON 1e-5

void test_lerp(void) {
    fand_assert(lerp(20ul, 30ul, 0.7f)  == 27ul);
    fand_assert(lerp(20ul, 30ul, 0.2f)  == 22ul);
    fand_assert(lerp(20ul, 30ul, 0.25f) == 23ul);
    fand_assert(lerp(20ul, 30ul, 1.1f)  == 31ul);
}

void test_lerp_inverse(void) {
    fand_assert(fabs(lerp_inverse(20ul, 30ul, 25ul) - 0.5f) < EPSILON);
    fand_assert(fabs(lerp_inverse(20ul, 30ul, 20ul) - 0.f)  < EPSILON);
    fand_assert(fabs(lerp_inverse(20ul, 30ul, 27ul) - 0.7f) < EPSILON);
    fand_assert(fabs(lerp_inverse(20ul, 30ul, 31ul) - 1.1f) < EPSILON);
}
