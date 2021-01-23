#include "hwmon_mock.h"
#include "mock.h"

#include <stdio.h>

static int(*write_pwm)(unsigned long) = 0;

void mock_hwmon_write_pwm(int(*mock)(unsigned long)) {
    mock_function(write_pwm, mock);
}

int hwmon_write_pwm(unsigned long pwm) {
    validate_mock(hwmon_write_pwm, write_pwm);
    return write_pwm(pwm);
}
