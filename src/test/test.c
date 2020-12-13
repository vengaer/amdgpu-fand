#include "test.h"

int failed_assertions = 0;
int total_assertions = 0;

int test_summary(void) {
    if(failed_assertions) {
        printf("%d/%d assertions failed\n", failed_assertions, total_assertions);
    }
    else {
        printf("Successfully finished %d asseritons\n", total_assertions);
    }

    return failed_assertions;
}
