#include "sensors.h"

#include <avr/io.h>
#include <math.h>

#include "fail.h"

int ntc_temp_c(float r_ntc)
{
  const float B = 3950.0f;
  const float T0 = 298.15f;

  float t_k = 1000.0f / (1.0f / T0 + logf(r_ntc) / B);
  return t_k - 273150;
}

inline void set_adc_channel(int channel);
inline void set_adc_channel(int channel)
{
  ADMUX = (1 << REFS0) | (channel & 0x07);
  if(channel & 0x08)
    ADCSRB |= (1 << MUX5);
  else
    ADCSRB &= ~(1 << MUX5);
}

int temperature_sensor(sensor_id_t id)
{
  switch(id) {
    case DRIVER_0_TEMPERATURE:
      set_adc_channel(0);
      break;
    case DRIVER_1_TEMPERATURE:
      set_adc_channel(1);
      break;
    case DRIVER_2_TEMPERATURE:
      set_adc_channel(2);
      break;
    case DRIVER_3_TEMPERATURE:
      set_adc_channel(3);
      break;

    default:
      return 0;
      break;
  }
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));

  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));
  float r_ntc_100k = (5115.0f / ADC - 1);
  return ntc_temp_c(r_ntc_100k);
}

int sensors_setup()
{
  ADMUX = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRA |= (1 << ADSC);

  ON_ERROR_ABORT(
      register_sensor(DRIVER_0_TEMPERATURE, temperature_sensor, 1000, 'C'));
  ON_ERROR_ABORT(
      register_sensor(DRIVER_1_TEMPERATURE, temperature_sensor, 1000, 'C'));
  ON_ERROR_ABORT(
      register_sensor(DRIVER_2_TEMPERATURE, temperature_sensor, 1000, 'C'));
  ON_ERROR_ABORT(
      register_sensor(DRIVER_3_TEMPERATURE, temperature_sensor, 1000, 'C'));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_0_TEMPERATURE, ANY, 10,
                                         MOTOR_0_FAULT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_0_TEMPERATURE, ANY, 70,
                                         DRIVER_0_OVERHEAT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_0_TEMPERATURE, ANY, 100,
                                         MOTOR_0_FAULT, 1, 1, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_1_TEMPERATURE, ANY, 10,
                                         MOTOR_1_FAULT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_1_TEMPERATURE, ANY, 70,
                                         DRIVER_1_OVERHEAT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_1_TEMPERATURE, ANY, 100,
                                         MOTOR_1_FAULT, 1, 1, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_2_TEMPERATURE, ANY, 10,
                                         MOTOR_2_FAULT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_2_TEMPERATURE, ANY, 70,
                                         DRIVER_2_OVERHEAT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_2_TEMPERATURE, ANY, 100,
                                         MOTOR_2_FAULT, 1, 1, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_3_TEMPERATURE, ANY, 10,
                                         MOTOR_3_FAULT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_3_TEMPERATURE, ANY, 70,
                                         DRIVER_3_OVERHEAT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_3_TEMPERATURE, ANY, 100,
                                         MOTOR_3_FAULT, 1, 1, 5, 10000));

  return 0;
}
