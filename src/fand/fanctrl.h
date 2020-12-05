#ifndef FANCTRL_H
#define FANCTRL_H

#include "config.h"

int fanctrl_init(struct fand_config *config);
int fanctrl_acquire(void);
int fanctrl_release(void);

#endif /* FANCTRL_H */
