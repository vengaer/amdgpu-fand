#include "fanctrl_mock.h"
#include "mock.h"

#include <stdio.h>

static int(*get_speed)(void) = 0;
static int(*get_temp)(void) = 0;

void mock_fanctrl_get_speed(int(*mock)(void)) {
    mock_function(get_speed, mock);
}

void mock_fanctrl_get_temp(int(*mock)(void)) {
    mock_function(get_temp, mock);
}

int fanctrl_get_speed(void) {
    validate_mock(fanctrl_get_speed, get_speed);
    return get_speed();
}

int fanctrl_get_temp(void) {
    validate_mock(fanctrl_get_temp, get_temp);
    return get_temp();
}
