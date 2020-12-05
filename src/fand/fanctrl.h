#ifndef FANCTRL_H
#define FANCTRL_H

#include "config.h"

int fanctrl_init(struct fand_config *config);
int fanctrl_acquire(void);
int fanctrl_release(void);
int fanctrl_adjust(void);
int fanctrl_get_speed(void);
int fanctrl_get_temp(void);

#endif /* FANCTRL_H */
