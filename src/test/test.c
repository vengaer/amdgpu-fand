#include "test.h"

#include <stddef.h>
#include <string.h>

enum { SECTION_MAX_LENGTH = 128 };

int failed_assertions = 0;
int total_assertions = 0;

void test_section(char const *name) {
    char separator[SECTION_MAX_LENGTH];
    memset(separator, '=', sizeof(separator));

    size_t len = strlen(name);
    if(len >= sizeof(separator)) {
        len = sizeof(separator) - 1;
    }
    separator[len] = '\0';

    printf("\n%s\n%s\n", name, separator);
}

int test_summary(void) {
    if(failed_assertions) {
        printf("\n%d/%d assertions failed\n", failed_assertions, total_assertions);
    }
    else {
        printf("\nSuccessfully finished %d assertions\n", total_assertions);
    }

    return failed_assertions;
}
