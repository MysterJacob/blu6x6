#include "motors.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#define PWM_CEILING 799
#define PWM_CEILING_T0 255
#define ENABLE_PORT PORTH
#define ENABLE_DDR DDRH
#define ENABLE_PIN PH6

struct motor_axis {
  int16_t target;
  int16_t speed;
};

static struct {
  struct motor_axis lhs;
  struct motor_axis rhs;
  uint16_t speed_cap;
  uint16_t accel_step;
  uint16_t decel_step;
} motor_cfg;

static int16_t clamp_i16(int32_t v, int16_t lo, int16_t hi)
{
  if(v < lo) return lo;
  if(v > hi) return hi;
  return (int16_t)v;
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
  int16_t target = m->target;
  int16_t speed = m->speed;
  uint16_t step;

  if(speed == target) return;

  if(speed > 0 && target < 0) {
    step = motor_cfg.decel_step;
    if((uint16_t)speed <= step)
      speed = 0;
    else
      speed -= (int16_t)step;
  } else if(speed < 0 && target > 0) {
    step = motor_cfg.decel_step;
    if((uint16_t)(-speed) <= step)
      speed = 0;
    else
      speed += (int16_t)step;
  } else if(target > speed) {
    step = speed >= 0 ? motor_cfg.accel_step : motor_cfg.decel_step;
    if((int32_t)target - speed <= step)
      speed = target;
    else
      speed += (int16_t)step;
  } else {
    step = speed <= 0 ? motor_cfg.accel_step : motor_cfg.decel_step;
    if((int32_t)speed - target <= step)
      speed = target;
    else
      speed -= (int16_t)step;
  }

  m->speed = clamp_i16(speed, -(int16_t)motor_cfg.speed_cap,
                       (int16_t)motor_cfg.speed_cap);
}

int motors_init(void)
{
  memset(&motor_cfg, 0, sizeof(motor_cfg));

  motor_cfg.speed_cap = PWM_CEILING;
  motor_cfg.accel_step = 2;
  motor_cfg.decel_step = 4;

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

int set_drive(int16_t lhs, int16_t rhs)
{
  motor_cfg.lhs.target = clamp_i16(lhs, -(int16_t)motor_cfg.speed_cap,
                                   (int16_t)motor_cfg.speed_cap);
  motor_cfg.rhs.target = clamp_i16(rhs, -(int16_t)motor_cfg.speed_cap,
                                   (int16_t)motor_cfg.speed_cap);
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
  return motor_cfg.lhs.speed || motor_cfg.rhs.speed || motor_cfg.lhs.target ||
         motor_cfg.rhs.target;
}

ISR(TIMER1_COMPA_vect)
{
  uint16_t lhs_pwm;
  uint16_t rhs_pwm;

  chase_axis(&motor_cfg.lhs);
  chase_axis(&motor_cfg.rhs);

  lhs_pwm = abs_i16(motor_cfg.lhs.speed);
  rhs_pwm = abs_i16(motor_cfg.rhs.speed);

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
