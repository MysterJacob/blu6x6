#include <stdint.h>

int motors_init();
int is_driving();
void motors_estop();
int set_drive(float lhs, float rhs);
