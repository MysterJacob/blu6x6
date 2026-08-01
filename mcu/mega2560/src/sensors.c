#include "sensors.h"

#include <avr/io.h>
#include <math.h>

#include "fail.h"

float ntc_temp_c(float r_ntc)
{
  float t0 = 298.15f;
  float r0 = 100000.0f;
  float b = 3950.0f;

  float t_kelvin = 1.0f / ((1.0f / t0) + (1.0f / b) * log(r_ntc / r0));
  float t_celsius = t_kelvin - 273.15f;
  return t_celsius * 10;
}

inline void set_adc_pin(int channel);
void set_adc_pin(int pin)
{
  ADMUX = (1 << REFS0) | (pin & 0x07);
  if(pin & 0x08)
    ADCSRB |= (1 << MUX5);
  else
    ADCSRB &= ~(1 << MUX5);
}

int32_t temperature_sensor(sensor_id_t id)
{
  switch(id) {
    case DRIVER_0_TEMPERATURE:
      set_adc_pin(0);
      break;
    case DRIVER_1_TEMPERATURE:
      set_adc_pin(1);
      break;
    case DRIVER_2_TEMPERATURE:
      set_adc_pin(2);
      break;
    case DRIVER_3_TEMPERATURE:
      set_adc_pin(3);
      break;
    case MOTOR_0_TEMPERATURE:
      set_adc_pin(4);
      break;
    case MOTOR_1_TEMPERATURE:
      set_adc_pin(5);
      break;
    case MOTOR_2_TEMPERATURE:
      set_adc_pin(6);
      break;
    case MOTOR_3_TEMPERATURE:
      set_adc_pin(7);
      break;

    default:
      return 0;
      break;
  }
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));

  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));
  const int adc = ADC;
  float r_ntc_100k = 100000.0f * ((1023.0f - (float)adc) / (float)adc);
  //   printf("id: %d r_ntc_100k: %f\n", id, r_ntc_100k);
  return ntc_temp_c(r_ntc_100k);
}

int32_t motor_power_sensor(sensor_id_t id)
{
  switch(id) {
    case MOTOR_0_POWER:
      return (PINC & _BV(PC6)) > 0;
      break;
    case MOTOR_1_POWER:
      return (PINC & _BV(PC7)) > 0;
      break;
    case MOTOR_2_POWER:
      return (PINA & _BV(PA7)) > 0;
      break;
    case MOTOR_3_POWER:
      return (PINA & _BV(PA6)) > 0;
      break;
    default:
      return 0;
  }
}

int32_t motor_current_sensor(sensor_id_t id)
{
  switch(id) {
    case MOTOR_0_CURRENT:
      set_adc_pin(11);
      break;
    case MOTOR_1_CURRENT:
      set_adc_pin(10);
      break;
    case MOTOR_2_CURRENT:
      set_adc_pin(9);
      break;
    case MOTOR_3_CURRENT:
      set_adc_pin(8);
      break;
    default:
      return 0;
      break;
  }
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));

  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));
  const int adc = ADC;
  return adc * 43.445;
}

int32_t battery_voltage_sensors(__attribute__((unused)) sensor_id_t id)
{
  set_adc_pin(12);
  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));

  ADCSRA |= (1 << ADSC);
  while(ADCSRA & (1 << ADSC));
  const int adc = ADC;
  return adc * 23.74;
}
const int motor_warning_temperature = 65;
const int motor_shutoff_temperature = 90;

int sensors_setup()
{
  ADMUX = (1 << REFS0);
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
  ADCSRA |= (1 << ADSC);
  DDRA &= ~(_BV(PA6) | _BV(PA7));
  DDRC &= ~(_BV(PC6) | _BV(PC7));

  ON_ERROR_ABORT(
      register_sensor(DRIVER_0_TEMPERATURE, temperature_sensor, 10, 'C'));
  ON_ERROR_ABORT(
      register_sensor(DRIVER_1_TEMPERATURE, temperature_sensor, 10, 'C'));
  ON_ERROR_ABORT(
      register_sensor(DRIVER_2_TEMPERATURE, temperature_sensor, 10, 'C'));
  ON_ERROR_ABORT(
      register_sensor(DRIVER_3_TEMPERATURE, temperature_sensor, 10, 'C'));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_0_TEMPERATURE, ANY, 10,
                                         MOTOR_0_FAULT, 1, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_0_TEMPERATURE, ANY,
                                         motor_warning_temperature,
                                         DRIVER_0_OVERHEAT, 1, 0, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_0_TEMPERATURE, ANY,
                                         motor_shutoff_temperature,
                                         MOTOR_0_FAULT, 1, 0, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_1_TEMPERATURE, ANY, 10,
                                         MOTOR_1_FAULT, 2, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_1_TEMPERATURE, ANY,
                                         motor_warning_temperature,
                                         DRIVER_1_OVERHEAT, 2, 0, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_1_TEMPERATURE, ANY,
                                         motor_shutoff_temperature,
                                         MOTOR_1_FAULT, 2, 0, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_2_TEMPERATURE, ANY, 10,
                                         MOTOR_2_FAULT, 4, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_2_TEMPERATURE, ANY,
                                         motor_warning_temperature,
                                         DRIVER_2_OVERHEAT, 4, 0, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_2_TEMPERATURE, ANY,
                                         motor_shutoff_temperature,
                                         MOTOR_2_FAULT, 4, 0, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_3_TEMPERATURE, ANY, 10,
                                         MOTOR_3_FAULT, 8, 1, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_3_TEMPERATURE, ANY,
                                         motor_warning_temperature,
                                         DRIVER_3_OVERHEAT, 8, 0, 5, 10000));
  ON_ERROR_ABORT(register_sensor_ratings(DRIVER_3_TEMPERATURE, ANY,
                                         motor_shutoff_temperature,
                                         MOTOR_3_FAULT, 8, 0, 5, 10000));

  ON_ERROR_ABORT(register_sensor(MOTOR_0_POWER, motor_power_sensor, 1, '*'));
  ON_ERROR_ABORT(register_sensor(MOTOR_1_POWER, motor_power_sensor, 1, '*'));
  ON_ERROR_ABORT(register_sensor(MOTOR_2_POWER, motor_power_sensor, 1, '*'));
  ON_ERROR_ABORT(register_sensor(MOTOR_3_POWER, motor_power_sensor, 1, '*'));

  ON_ERROR_ABORT(register_sensor_ratings(MOTOR_0_POWER, ARMED, 1,
                                         MOTOR_0_FAULT, 1, 1, 1, 500));
  ON_ERROR_ABORT(register_sensor_ratings(MOTOR_1_POWER, ARMED, 1,
                                         MOTOR_1_FAULT, 2, 1, 1, 500));
  ON_ERROR_ABORT(register_sensor_ratings(MOTOR_2_POWER, ARMED, 1,
                                         MOTOR_2_FAULT, 4, 1, 1, 500));
  ON_ERROR_ABORT(register_sensor_ratings(MOTOR_3_POWER, ARMED, 1,
                                         MOTOR_3_FAULT, 8, 1, 1, 500));

  ON_ERROR_ABORT(
      register_sensor(MOTOR_0_CURRENT, motor_current_sensor, 1000, 'A'));
  ON_ERROR_ABORT(
      register_sensor(MOTOR_1_CURRENT, motor_current_sensor, 1000, 'A'));
  ON_ERROR_ABORT(
      register_sensor(MOTOR_2_CURRENT, motor_current_sensor, 1000, 'A'));
  ON_ERROR_ABORT(
      register_sensor(MOTOR_3_CURRENT, motor_current_sensor, 1000, 'A'));

  ON_ERROR_ABORT(
      register_sensor(BATTERY_VOLTAGE, battery_voltage_sensors, 1000, 'V'));

  ON_ERROR_ABORT(register_sensor_ratings(BATTERY_VOLTAGE, ANY, 20.5,
                                         BATTERY_LOW, 1, 1, 5, 10000));

  ON_ERROR_ABORT(register_sensor_ratings(BATTERY_VOLTAGE, ARMED, 18.5,
                                         BATTERY_CRITICAL, 1, 1, 5, 1000));

  return 0;
}
