#ifndef TEST_H
#define TEST_H

#include <stdio.h>

#define fand_assert(condition)                                                  \
    do {                                                                        \
        ++total_assertions;                                                     \
        if(!(condition)) {                                                      \
            printf("Assertion '%s' on line %d failed\n", #condition, __LINE__); \
            ++failed_assertions;                                                \
        }                                                                       \
    } while(0)

extern int failed_assertions;
extern int total_assertions;

int test_summary(void);

#endif /* TEST_H */
