#include "test.h"
#include "test_sha1.h"

int main(void) {
    test_sha1();

    if(failed_assertions) {
        printf("%d/%d assertions failed\n", failed_assertions, total_assertions);
    }
    else {
        printf("Successfully finished %d asseritons\n", total_assertions);
    }

    return failed_assertions;
}
