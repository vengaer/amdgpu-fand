#ifndef FANCTRL_H
#define FANCTRL_H

#include "config.h"

#include <stdbool.h>

int fanctrl_init(bool init_dryrun);
int fanctrl_release(void);
int fanctrl_configure(struct fand_config *config);
int fanctrl_adjust(void);
int fanctrl_get_speed(void);
int fanctrl_get_temp(void);

#endif /* FANCTRL_H */
