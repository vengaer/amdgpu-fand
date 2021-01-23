#ifndef MOCK_H
#define MOCK_H

#include "macro.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef FAND_TEST_CONFIG
#define MOCKABLE(...)
#else
#define MOCKABLE(...) __VA_ARGS__
#endif

#define validate_mock(function, mock)                           \
    do {                                                        \
        if(!mock) {                                             \
            fprintf(stderr, "%s is not mocked\n", #function);   \
            abort();                                            \
        }                                                       \
    } while(0)


void mock_guard_init(void);
void mock_guard_add(void *addr, size_t size);
bool mock_guard_active(void);
void mock_guard_cleanup(void);

#define mock_guard                                                  \
    for(int CAT_EXPAND(mmock_gd_,__LINE__) = (mock_guard_init(),0); \
        CAT_EXPAND(mmock_gd_,__LINE__) < 1;                         \
        mock_guard_cleanup(), ++CAT_EXPAND(mmock_gd_,__LINE__))

#define mock_function(mockptr, mock)                    \
    do {                                                \
        mockptr = mock;                                 \
        if(mock_guard_active()) {                       \
            mock_guard_add(&mockptr, sizeof(mockptr));  \
        }                                               \
    } while(0)

#endif /* MOCK_H */
