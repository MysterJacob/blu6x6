#include "motors.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <string.h>

// Enable PH6 - D9
// PWM Forward Right D2 - PE4 OCR3B
// PWM Forward Left  D3 - PE5 OCR3C
// PWM Reverse Right D7 - PH4 OCR4B
// PWM Reverse Left  D8 - PH5 OCR4C

#define PWM_FR OCR3B
#define PWM_FL OCR3C
#define PWM_BR OCR4B
#define PWM_BL OCR4C

#define PWM_CEILING 799
#define PWM_CEILING_T0 255

#define ENABLE_PORT PORTH
#define ENABLE_DDR DDRH
#define ENABLE_PIN PH6

struct motor_axis {
  float target;
  float speed;
};

static struct {
  struct motor_axis lhs;
  struct motor_axis rhs;
  float speed_cap;
  float accel_step;
  float decel_step;
} motor_cfg;

static float clampf(float v, float lo, float hi)
{
  if(v < lo) return lo;
  if(v > hi) return hi;
  return (float)v;
}

static void chase_axis(volatile struct motor_axis *m)
{
  float target = m->target;
  float speed = m->speed;
  float step;

  if(speed == target) return;

  if(speed > 0 && target < 0) {
    step = motor_cfg.decel_step;
    if(speed <= step)
      speed = 0;
    else
      speed -= step;
  } else if(speed < 0 && target > 0) {
    step = motor_cfg.decel_step;
    if((-speed) <= step)
      speed = 0;
    else
      speed += step;
  } else if(target > speed) {
    step = speed >= 0 ? motor_cfg.accel_step : motor_cfg.decel_step;
    if(target - speed <= step)
      speed = target;
    else
      speed += step;
  } else {
    step = speed <= 0 ? motor_cfg.accel_step : motor_cfg.decel_step;
    if(speed - target <= step)
      speed = target;
    else
      speed -= step;
  }

  m->speed = clampf(speed, -(int16_t)motor_cfg.speed_cap,
                    (int16_t)motor_cfg.speed_cap);
}

int motors_init(void)
{
  memset(&motor_cfg, 0, sizeof(motor_cfg));

  motor_cfg.speed_cap = 100;
  motor_cfg.accel_step = 0.3;
  motor_cfg.decel_step = 0.1;

  DDRE |= _BV(PE4) | _BV(PE5);
  DDRH |= _BV(PH4) | _BV(PH5);

  ENABLE_DDR |= _BV(ENABLE_PIN);
  ENABLE_PORT &= ~_BV(ENABLE_PIN);

  TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1) | _BV(WGM31);
  TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS30);
  ICR3 = PWM_CEILING;

  TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(COM4C1) | _BV(WGM41);
  TCCR4B = _BV(WGM43) | _BV(WGM42) | _BV(CS40);
  ICR4 = PWM_CEILING;

  PWM_FR = 0;
  PWM_FL = 0;
  PWM_BR = 0;
  PWM_BL = 0;

  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11);
  TCNT1 = 0;
  OCR1A = 9999;
  TIMSK1 = _BV(OCIE1A);

  sei();
  return 0;
}

int set_drive(float lhs, float rhs)
{
  motor_cfg.lhs.target = clampf(lhs, -motor_cfg.speed_cap, motor_cfg.speed_cap);
  motor_cfg.rhs.target = clampf(rhs, -motor_cfg.speed_cap, motor_cfg.speed_cap);
  return 0;
}

void motors_estop(void)
{
  motor_cfg.lhs.target = 0;
  motor_cfg.rhs.target = 0;
  motor_cfg.lhs.speed = 0;
  motor_cfg.rhs.speed = 0;

  PWM_FR = 0;
  PWM_FL = 0;
  PWM_BR = 0;
  PWM_BL = 0;

  ENABLE_PORT &= ~_BV(ENABLE_PIN);
}

int is_driving(void)
{
  return fabs(motor_cfg.lhs.speed) > 0 || fabs(motor_cfg.rhs.speed) > 0 ||
         fabs(motor_cfg.lhs.target) > 0 || fabs(motor_cfg.rhs.target) > 0;
}

float speed_map(float s)
{
  return 10 + s * 0.9;
}

ISR(TIMER1_COMPA_vect)
{
  uint16_t lhs_pwm;
  uint16_t rhs_pwm;

  chase_axis(&motor_cfg.lhs);
  chase_axis(&motor_cfg.rhs);

  lhs_pwm = PWM_CEILING * speed_map(fabs(motor_cfg.lhs.speed)) / 100.0f;
  rhs_pwm = PWM_CEILING * speed_map(fabs(motor_cfg.rhs.speed)) / 100.0f;

  if(lhs_pwm == 0 && rhs_pwm == 0 && motor_cfg.lhs.target == 0 &&
     motor_cfg.rhs.target == 0) {
    ENABLE_PORT &= ~_BV(ENABLE_PIN);
  } else {
    ENABLE_PORT |= _BV(ENABLE_PIN);
  }

  if(motor_cfg.lhs.speed > 0) {
    PWM_FL = lhs_pwm;
    PWM_BL = 0;
  } else if(motor_cfg.lhs.speed < 0) {
    PWM_FL = 0;
    PWM_BL = lhs_pwm;
  } else {
    PWM_FL = 0;
    PWM_BL = 0;
  }

  if(motor_cfg.rhs.speed > 0) {
    PWM_FR = rhs_pwm;
    PWM_BR = 0;
  } else if(motor_cfg.rhs.speed < 0) {
    PWM_FR = 0;
    PWM_BR = rhs_pwm;
  } else {
    PWM_FR = 0;
    PWM_BR = 0;
  }
}
