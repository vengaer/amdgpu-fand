#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdint.h>

#define LOG(level, ...)             \
    {                               \
        extern uint8_t log_level;   \
        if(log_level >= level) {    \
            printf(__VA_ARGS__);    \
        }                           \
    }

#endif