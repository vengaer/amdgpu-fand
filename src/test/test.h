#ifndef TEST_H
#define TEST_H

#include "macro.h"

#include <stdio.h>

#define fand_assert(condition)                                                  \
    do {                                                                        \
        ++total_assertions;                                                     \
        if(!(condition)) {                                                      \
            printf("Assertion '%s' on line %d failed\n", #condition, __LINE__); \
            ++failed_assertions;                                                \
        }                                                                       \
    } while(0)

#define run(test)                                                                       \
    do {                                                                                \
        int CAT_EXPAND(prev_assertions_,__LINE__) = failed_assertions;                  \
        printf("%-30s --> ", #test);                                                    \
        test();                                                                         \
        printf("%s\n", CAT_EXPAND(prev_assertions_,__LINE__) == failed_assertions ?     \
                "ok" : "fail");                                                         \
    } while(0);

#define section(name)   \
    test_section(#name)

extern int failed_assertions;
extern int total_assertions;

void test_section(char const *name);
int test_summary(void);

#endif /* TEST_H */
