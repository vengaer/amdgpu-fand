#include "fanctrl_mock.h"

#include <stdio.h>

static int(*get_speed)(void) = 0;
static int(*get_temp)(void) = 0;

void mock_fanctrl_get_speed(int(*mock)(void)) {
    get_speed = mock;
}

void mock_fanctrl_get_temp(int(*mock)(void)) {
    get_temp = mock;
}

int fanctrl_get_speed(void) {
    if(!get_speed) {
        fputs("fanctrl_get_speed not mocked\n", stderr);
        return -1;
    }
    return get_speed();
}

int fanctrl_get_temp(void) {
    if(!get_temp) {
        fputs("fanctrl_get_temp not mocked\n", stderr);
        return -1;
    }
    return get_temp();
}
