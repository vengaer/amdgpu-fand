#ifndef MOCK_FANCTRL_H
#define MOCK_FANCTRL_H

void mock_fanctrl_get_speed(int(*mock)(void));
void mock_fanctrl_get_temp(int(*mock)(void));

int fanctrl_get_speed(void);
int fanctrl_get_temp(void);

#endif /* MOCK_FANCTRL_H */
