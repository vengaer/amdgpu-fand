#ifndef CACHE_H
#define CACHE_H

#include "defs.h"

struct fand_cache {
    char pwm[HWMON_PATH_SIZE];
    char pwm_enable[HWMON_PATH_SIZE];
    char temp_input[HWMON_PATH_SIZE];
    unsigned card_idx;
};

extern struct fand_cache fand_cache;

int cache_load(void);
int cache_write(void);


#endif /* CACHE_H */
