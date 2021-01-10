#include "config.h"
#include "fanctrl.h"
#include "fanctrl_test.h"
#include "fanctrl_mock.h"
#include "hwmon_mock.h"
#include "interpolation.h"
#include "test.h"

#include <math.h>
#include <stdbool.h>

#define MAX_PWM 255.f

static int temp = -1;
static int speed = -1;

static unsigned long pwm;

static int get_speed(void) {
    return speed;
}

static int get_temp(void) {
    return temp;
}

static int write_pwm(unsigned long value) {
    pwm =  value;
    return 0;
}

void test_fanctrl_adjust(void) {
    mock_fanctrl_get_speed(get_speed);
    mock_fanctrl_get_temp(get_temp);
    mock_hwmon_write_pwm(write_pwm);

    struct fand_config config = {
        .throttle = true,
        .matrix_rows = 3,
        .hysteresis = 3,
        .interval = 2,
        .matrix = {
            50, 20, 60, 50, 80, 100
        }
    };

    fand_assert(fanctrl_configure(&config) == 0);

    temp = 40;

    fand_assert(fanctrl_adjust() == 0);
    fand_assert(pwm == 0);

    temp = 55;

    fand_assert(fanctrl_adjust() == 0);
    fand_assert(pwm == (unsigned long)round(MAX_PWM * 0.35f));

    temp = 70;

    fand_assert(fanctrl_adjust() == 0);
    fand_assert(pwm == (unsigned long)round(MAX_PWM * 0.75f));

    temp = 58;

    fand_assert(fanctrl_adjust() == 0);
    fand_assert(pwm == (unsigned long)round(MAX_PWM * 0.5f));

    temp = 57;

    fand_assert(fanctrl_adjust() == 0);
    fand_assert(pwm == (unsigned long)round(MAX_PWM * 0.41f));

    config.throttle = false;
    fand_assert(fanctrl_configure(&config) == 0);

    temp = 45;
    fand_assert(fanctrl_adjust() == 0);
    fand_assert(pwm == (unsigned long)round(MAX_PWM * 0.2f));
}
