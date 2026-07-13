#include "sensors.h"

#include <avr/io.h>

int ad0_sensor()
{
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC))
    ;
  return ADC;
}

int sensors_setup()
{
  ADMUX = (1 << REFS0);
  /* Enable ADC, prescaler /128 → 16MHz/128 = 125kHz (within 50-200kHz range) */
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  /* First conversion is slower (25 cycles) — discard it */
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC))
    ;

  register_sensor(BATTERY_VOLTAGE, ad0_sensor, 1023, 'V');
  return 0;
}
