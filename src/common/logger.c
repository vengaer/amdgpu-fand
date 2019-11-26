#include "logger.h"

#include <stdint.h>

uint8_t verbosity_level = 0;

void set_verbosity_level(uint8_t verbosity) {
    if(verbosity > MAX_VERBOSITY) {
        fprintf(stderr, "Warning, max log level is %d\n", MAX_VERBOSITY);
        verbosity = MAX_VERBOSITY;
    }
    verbosity_level = verbosity;
}
