#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdint.h>

#define VERBOSITY_LVL1 1
#define VERBOSITY_LVL2 2
#define VERBOSITY_LVL3 3
#define MAX_VERBOSITY 3

#define LOG(level, ...)                             \
    {                                               \
        extern uint8_t verbosity_level;             \
        if(verbosity_level >= level) {              \
            if(verbosity_level >= VERBOSITY_LVL3) { \
                printf("%s: ", __FILE__);           \
            }                                       \
            printf(__VA_ARGS__);                    \
        }                                           \
    }

#define E_LOG(level, ...)                           \
    {                                               \
        extern uint8_t verbosity_level;             \
        if(verbosity_level >= level) {              \
            if(verbosity_level >= VERBOSITY_LVL3) { \
                printf("%s: ", __FILE__);           \
            }                                       \
            fprintf(stderr, __VA_ARGS__);           \
        }                                           \
    }

#define IPC_REQUEST_FMT(request)                    \
    "%s %s %d\n",                                   \
        (request)->type == ipc_get ?                \
            "get" : "set",                          \
        (request)->target == ipc_temp ?             \
            "temp" : "speed",                       \
        (request)->value
        

#endif
