#include <stdint.h>

int motors_init();
int is_driving();
void motors_estop();
int set_drive(int16_t lhs, int16_t rhs);
