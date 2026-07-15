#include "motors.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>

#define PWM_CELING 799

static struct {
  uint16_t max_acceleration;
  uint16_t max_deceleration;
  uint16_t speed_cap;
  uint16_t lhs_speed;
  uint16_t rhs_speed;
  int16_t lhs_request;
  int16_t rhs_request;

  int8_t lhs_dir;
  int8_t rhs_dir;
} motor_cfg;

int motors_init(void)
{
  memset(&motor_cfg, 0, sizeof(motor_cfg));
  motor_cfg.speed_cap = PWM_CELING;
  motor_cfg.max_acceleration = 1;
  motor_cfg.max_deceleration = 1;

  DDRE |= _BV(PE4) | _BV(PE5) | _BV(PE3);
  DDRH |= _BV(PH3) | _BV(PH4) | _BV(PH5);

  DDRH |= _BV(PH6);
  DDRB |= _BV(PB4) | _BV(PB5) | _BV(PB6);

  TCCR3A = _BV(COM3A1) | _BV(COM3B1) | _BV(COM3C1) | _BV(WGM31);
  TCCR3B = _BV(WGM33) | _BV(WGM32) | _BV(CS30);

  TCCR4A = _BV(COM4A1) | _BV(COM4B1) | _BV(COM4C1) | _BV(WGM41);
  TCCR4B = _BV(WGM43) | _BV(WGM42) | _BV(CS40);

  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11);
  TCNT1 = 0;
  OCR1A = 9999;
  TIMSK1 = _BV(OCIE1A);

  ICR3 = PWM_CELING;
  ICR4 = PWM_CELING;

  OCR3A = 0;
  OCR3B = 0;
  OCR3C = 0;
  OCR4A = 0;
  OCR4B = 0;
  OCR4C = 0;

  motor_cfg.lhs_request = 0;
  motor_cfg.rhs_request = 0;
  motor_cfg.lhs_dir = 0;
  motor_cfg.rhs_dir = 0;
  motor_cfg.lhs_speed = 0;
  motor_cfg.rhs_speed = 0;

  sei();
  return 0;
}

int set_drive(int16_t lhs, int16_t rhs)
{
  motor_cfg.lhs_request = lhs;
  motor_cfg.rhs_request = rhs;
  return 0;
}

void motors_estop(void)
{
  motor_cfg.lhs_request = 0;
  motor_cfg.rhs_request = 0;

  motor_cfg.lhs_speed = 0;
  motor_cfg.rhs_speed = 0;

  motor_cfg.lhs_dir = 0;
  motor_cfg.rhs_dir = 0;

  PORTH &= ~_BV(PH6);
  PORTB &= ~_BV(PB4);
  PORTB &= ~_BV(PB6);
  PORTB &= ~_BV(PB5);

  OCR3A = 0;
  OCR3B = 0;
  OCR3C = 0;
  OCR4A = 0;
  OCR4B = 0;
  OCR4C = 0;
}

int is_driving()
{
  return OCR3A | OCR3B | OCR3C | OCR4A | OCR4B | OCR4C;
}

ISR(TIMER1_COMPA_vect)
{
  uint16_t lhs_target = motor_cfg.lhs_request >= 0
                          ? (uint16_t)motor_cfg.lhs_request
                          : (uint16_t)(-motor_cfg.lhs_request);
  uint16_t rhs_target = motor_cfg.rhs_request >= 0
                          ? (uint16_t)motor_cfg.rhs_request
                          : (uint16_t)(-motor_cfg.rhs_request);

  if(lhs_target > motor_cfg.speed_cap) lhs_target = motor_cfg.speed_cap;
  if(rhs_target > motor_cfg.speed_cap) rhs_target = motor_cfg.speed_cap;

  if(motor_cfg.lhs_speed == 0) {
    int8_t desired = 0;
    if(motor_cfg.lhs_request > 0)
      desired = 1;
    else if(motor_cfg.lhs_request < 0)
      desired = -1;
    if(desired != motor_cfg.lhs_dir) {
      motor_cfg.lhs_dir = desired;
      if(motor_cfg.lhs_dir > 0) {
        PORTH |= _BV(PH6);
        PORTB &= ~_BV(PB4);
      } else if(motor_cfg.lhs_dir < 0) {
        PORTH &= ~_BV(PH6);
        PORTB |= _BV(PB4);
      } else {
        PORTH &= ~_BV(PH6);
        PORTB &= ~_BV(PB4);
      }
    }
  }

  if(motor_cfg.rhs_speed == 0) {
    int8_t desired = 0;
    if(motor_cfg.rhs_request > 0)
      desired = 1;
    else if(motor_cfg.rhs_request < 0)
      desired = -1;
    if(desired != motor_cfg.rhs_dir) {
      motor_cfg.rhs_dir = desired;
      if(motor_cfg.rhs_dir > 0) {
        PORTB |= _BV(PB6);
        PORTB &= ~_BV(PB5);
      } else if(motor_cfg.rhs_dir < 0) {
        PORTB &= ~_BV(PB6);
        PORTB |= _BV(PB5);
      } else {
        PORTB &= ~_BV(PB6);
        PORTB &= ~_BV(PB5);
      }
    }
  }

  if(motor_cfg.lhs_speed < lhs_target) {
    uint16_t d = lhs_target - motor_cfg.lhs_speed;
    if(d > motor_cfg.max_acceleration) d = motor_cfg.max_acceleration;
    motor_cfg.lhs_speed += d;
  } else if(motor_cfg.lhs_speed > lhs_target) {
    uint16_t d = motor_cfg.lhs_speed - lhs_target;
    if(d > motor_cfg.max_deceleration) d = motor_cfg.max_deceleration;
    motor_cfg.lhs_speed -= d;
  }

  if(motor_cfg.rhs_speed < rhs_target) {
    uint16_t d = rhs_target - motor_cfg.rhs_speed;
    if(d > motor_cfg.max_acceleration) d = motor_cfg.max_acceleration;
    motor_cfg.rhs_speed += d;
  } else if(motor_cfg.rhs_speed > rhs_target) {
    uint16_t d = motor_cfg.rhs_speed - rhs_target;
    if(d > motor_cfg.max_deceleration) d = motor_cfg.max_deceleration;
    motor_cfg.rhs_speed -= d;
  }

  if(motor_cfg.lhs_dir == 0) {
    OCR3A = 0;
    OCR3B = 0;
    OCR3C = 0;
  } else {
    if(motor_cfg.lhs_speed > PWM_CELING) motor_cfg.lhs_speed = PWM_CELING;
    OCR3A = motor_cfg.lhs_speed;
    OCR3B = motor_cfg.lhs_speed;
    OCR3C = motor_cfg.lhs_speed;
  }

  if(motor_cfg.rhs_dir == 0) {
    OCR4A = 0;
    OCR4B = 0;
    OCR4C = 0;
  } else {
    if(motor_cfg.rhs_speed > PWM_CELING) motor_cfg.rhs_speed = PWM_CELING;
    OCR4A = motor_cfg.rhs_speed;
    OCR4B = motor_cfg.rhs_speed;
    OCR4C = motor_cfg.rhs_speed;
  }
}
