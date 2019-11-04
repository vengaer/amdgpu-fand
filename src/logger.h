#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdint.h>

#define LOG_LV1 1
#define LOG_LV2 2
#define MAX_LOG_LV 2

#define LOG(level, ...)             \
    {                               \
        extern uint8_t log_level;   \
        if(log_level >= level) {    \
            printf(__VA_ARGS__);    \
        }                           \
    }

#endif
