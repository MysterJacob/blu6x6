#include "sensors.h"
int getBatteryVoltage()
{
  return 24000;
}
int getMCUVoltage()
{
  return 5000;
}

int getMotorForwardCurrent(int motor)
{
  return 0;
}
int getMotorPower(int motor)
{
  return 1;
}
