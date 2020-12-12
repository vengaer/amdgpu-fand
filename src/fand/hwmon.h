#ifndef HWMON_H
#define HWMON_H

int hwmon_open(void);
int hwmon_close(void);
int hwmon_read_temp(void);
int hwmon_read_pwm(void);
int hwmon_write_pwm(unsigned long pwm);

#endif /* HWMON_H */
