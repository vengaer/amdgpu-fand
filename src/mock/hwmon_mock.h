#ifndef MOCK_HWMON_H
#define MOCK_HWMON_H

void mock_hwmon_write_pwm(int(*mock)(unsigned long));

#endif /* MOCK_HWMON_H */
