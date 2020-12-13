#ifndef DAEMON_H
#define DAEMON_H

#include <stdbool.h>

int daemon_main(bool fork, bool dryryn, char const *config);

#endif /* DAEMON_H */
