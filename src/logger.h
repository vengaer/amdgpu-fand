#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdint.h>

#define VERBOSITY_LVL1 1
#define VERBOSITY_LVL2 2
#define VERBOSITY_LVL3 3
#define MAX_VERBOSITY 3

#define LOG(level, ...)                   \
    {                                     \
        extern uint8_t verbosity_level;   \
        if(verbosity_level >= level) {    \
            printf(__VA_ARGS__);          \
        }                                 \
    }

#endif
