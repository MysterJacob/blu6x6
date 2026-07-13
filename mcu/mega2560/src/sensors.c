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

  register_sensor_ratings(BATTERY_VOLTAGE, ANY, 0.5, OBDC_TEST_SOFT, 0, 1, 5,
                          100);
  register_sensor_ratings(BATTERY_VOLTAGE, ANY, 0.1, OBDC_TEST_HARD, 0, 1, 5,
                          100);
  return 0;
}
