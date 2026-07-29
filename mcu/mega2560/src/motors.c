#include "motors.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>
#include <string.h>

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

static int16_t clamp_i16(int32_t v, int16_t lo, int16_t hi)
{
  if(v < lo) return lo;
  if(v > hi) return hi;
  return (int16_t)v;
}

static float clampf(float v, float lo, float hi)
{
  if(v < lo) return lo;
  if(v > hi) return hi;
  return (float)v;
}

static uint16_t abs_i16(int16_t v)
{
  return v >= 0 ? (uint16_t)v : (uint16_t)(-v);
}

static uint8_t pwm0_from_pwm3(uint16_t v)
{
  if(v >= PWM_CEILING) return PWM_CEILING_T0;
  return (uint8_t)(((uint32_t)v * PWM_CEILING_T0) / PWM_CEILING);
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
  motor_cfg.decel_step = 0.3;

  DDRE |= _BV(PE3) | _BV(PE4) | _BV(PE5);
  DDRG |= _BV(PG5);
  ENABLE_DDR |= _BV(ENABLE_PIN);

  OCR3A = 0;
  OCR3B = 0;
  OCR3C = 0;
  OCR0B = 0;

  TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1) | _BV(WGM31);
  TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS30);
  ICR3 = PWM_CEILING;

  TCCR0A = _BV(COM0B1) | _BV(WGM01) | _BV(WGM00);
  TCCR0B = _BV(CS00);

  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11);
  TCNT1 = 0;
  OCR1A = 9999;
  TIMSK1 = _BV(OCIE1A);

  ENABLE_PORT &= ~_BV(ENABLE_PIN);

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

  OCR3A = 0;
  OCR3B = 0;
  OCR3C = 0;
  OCR0B = 0;

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
    OCR3C = lhs_pwm;
    OCR3A = 0;
  } else if(motor_cfg.lhs.speed < 0) {
    OCR3C = 0;
    OCR3A = lhs_pwm;
  } else {
    OCR3C = 0;
    OCR3A = 0;
  }

  if(motor_cfg.rhs.speed > 0) {
    OCR3B = rhs_pwm;
    OCR0B = 0;
  } else if(motor_cfg.rhs.speed < 0) {
    OCR3B = 0;
    OCR0B = pwm0_from_pwm3(rhs_pwm);
  } else {
    OCR3B = 0;
    OCR0B = 0;
  }
}
