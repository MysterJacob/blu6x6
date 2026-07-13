#include "system.h"

#include "avr/interrupt.h"

sys_t sys = {0};

int system_init()
{
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS22);
  TCNT2 = 6;
  OCR2A = 249;
  TIMSK2 = _BV(OCIE2A);

  sei();
  return 0;
}

ISR(TIMER2_COMPA_vect)
{
  sys.ms_from_boot += 1;
}

uint64_t get_ms_from_boot()
{
  cli();
  uint64_t time = sys.ms_from_boot;
  sei();
  return time;
}
