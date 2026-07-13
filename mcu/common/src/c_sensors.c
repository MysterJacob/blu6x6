#include <stdio.h>
#include <string.h>

#include "fail.h"
#include "port.h"
#include "sensors.h"

static struct {
  sensor_t sensors[_TOTAL_SENSOR_COUNT];
  sensor_t *sensors_lookup[_TOTAL_SENSOR_COUNT];
  size_t sensor_count;

  sensor_ratings_t ratings[MAX_SENSOR_RATINGS];
  size_t rating_count;
} _sensors;

static struct {
  uint64_t last_poll_ms;
  uint8_t samples;
} _sensors_obd[MAX_SENSOR_RATINGS];

int port_print_sensors_handler(int argc, char **argv);
int sensors_init()
{
  memset(&_sensors, 0, sizeof(_sensors));
  memset(_sensors_obd, 0, sizeof(_sensors_obd));
  ON_ERROR_ABORT(port_register_command("sensors", port_print_sensors_handler));
  return 0;

}

int register_sensor(sensor_id_t id, sensor_handler_t handler, int div)
{
  if(_sensors.sensor_count >= _TOTAL_SENSOR_COUNT) return 1;
  if(_sensors.sensors_lookup[id] != 0) return 2;
  if(div == 0) return 3;

  _sensors.sensors[_sensors.sensor_count] = (sensor_t){id, handler, div};
  _sensors.sensors_lookup[id] = &_sensors.sensors[_sensors.sensor_count++];
  return 0;
}

int register_sensor_ratings(sensor_id_t id, system_state_t state, float rating,
                            obd_code_t obd_code, uint8_t obd_fault,
                            uint8_t is_minimum, uint8_t required_samples,
                            int pool_ms)
{
  if(_sensors.rating_count >= MAX_SENSOR_RATINGS) return 1;
  if(required_samples == 0) return 2;
  _sensors.ratings[_sensors.rating_count++] =
      (sensor_ratings_t){id,        state,      rating,           obd_code,
                         obd_fault, is_minimum, required_samples, pool_ms};
  return 0;
}

int get_sensor_reading(sensor_id_t id, float *out)
{
  if(_sensors.sensors_lookup[id] == 0) return 1;
  int raw;
  int ecode = get_sensor_reading_raw(id, &raw);
  if(ecode != 0) return ecode;
  *out = (float)raw / _sensors.sensors_lookup[id]->div;
  return 0;
}

int get_sensor_reading_raw(sensor_id_t id, int *out)
{
  if(_sensors.sensors_lookup[id] == 0) return 1;
  *out = _sensors.sensors_lookup[id]->handler();
  return 0;
}

void perform_sensors_obd()
{
  const uint64_t time = get_ms_from_boot();
  for(size_t i = 0; i < _sensors.rating_count; i++) {
    sensor_ratings_t *rating = &_sensors.ratings[i];

    if(rating->system_state != ANY &&
       rating->system_state != get_system_state())
      continue;

    if(time - _sensors_obd[i].last_poll_ms < rating->pool_ms) continue;

    _sensors_obd[i].last_poll_ms = time;

    float reading;
    int err = get_sensor_reading(rating->id, &reading);

    int samples = _sensors_obd[i].samples;
    if(err != 0 || (rating->is_minimum == 1 && reading < rating->rating) ||
       (rating->is_minimum == 0 && reading > rating->rating)) {
      samples += 1;
      if(samples == rating->required_samples)
        obd_fault(rating->obd_code, rating->obd_fault);
    } else {
      samples -= 1;
      if(samples == 0) obd_clear(rating->obd_code);
    }
    if(samples >= 0 && samples <= rating->required_samples)
      _sensors_obd[i].samples = samples;
  }
}

int port_print_sensors_handler(__attribute__((unused)) int argc, __attribute__((unused)) char **argv)
{
  for(size_t i = 0; i < _sensors.sensor_count; i++) {
    const sensor_t *sensor = _sensors.sensors;
    float out;
    if(get_sensor_reading(sensor->id, &out) == 0) {
      printf("%s \t%-10f\n", sensor_id_to_string(sensor->id), out);
    } else {
      printf("%-16d READ FAULT\n", sensor->id);
    }
  }
  return 0;
}

const char *sensor_id_to_string(sensor_id_t id)
{
  switch(id) {
    case BATTERY_VOLTAGE:
      return "BATTERY_VOLTAGE";
    case MCU_VOLTAGE:
      return "MCU_VOLTAGE";
    case MOTOR_1_POWER:
      return "MOTOR_1_POWER";
    case MOTOR_1_CURRENT:
      return "MOTOR_1_CURRENT";
    case MOTOR_2_POWER:
      return "MOTOR_2_POWER";
    case MOTOR_2_CURRENT:
      return "MOTOR_2_CURRENT";
    case MOTOR_3_POWER:
      return "MOTOR_3_POWER";
    case MOTOR_3_CURRENT:
      return "MOTOR_3_CURRENT";
    case MOTOR_4_POWER:
      return "MOTOR_4_POWER";
    case MOTOR_4_CURRENT:
      return "MOTOR_4_CURRENT";
    case MOTOR_5_POWER:
      return "MOTOR_5_POWER";
    case MOTOR_5_CURRENT:
      return "MOTOR_5_CURRENT";
    case _TOTAL_SENSOR_COUNT:
      return "_TOTAL_SENSOR_COUNT";
    default:
      return "UNKNOWN_SENSOR_ID";
  }
}
