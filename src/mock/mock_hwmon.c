#include "mock_hwmon.h"

#include <stdio.h>

static int(*write_pwm)(unsigned long);

void mock_hwmon_write_pwm(int(*mock)(unsigned long)) {
    write_pwm = mock;
}

int hwmon_write_pwm(unsigned long pwm) {
    if(!write_pwm) {
        fputs("hwmon_write_pwm not mocked\n", stderr);
        return -1;
    }
    return write_pwm(pwm);
}
