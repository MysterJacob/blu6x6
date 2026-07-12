#include "sensors.h"

#include <string.h>

static struct {
  sensor_t sensors[_TOTAL_SENSOR_COUNT];
  sensor_t *sensors_lookup[_TOTAL_SENSOR_COUNT];
  int sensor_count;

  sensor_ratings_t ratings[MAX_SENSOR_RATINGS];
  int rating_count;
} _sensors;

int sensors_init()
{
  memset(_sensors.sensors, 0, sizeof(_sensors.sensors));
  memset(_sensors.sensors_lookup, 0, sizeof(_sensors.sensors));
  memset(_sensors.ratings, 0, sizeof(_sensors.ratings));
  return 0;
}

int register_sensor(sensor_id_t id, sensor_handler_t handler, int div)
{
  if(_sensors.sensor_count >= _TOTAL_SENSOR_COUNT) return 1;
  if(_sensors.sensors_lookup[id] != 0) return 2;
  _sensors.sensors[_sensors.sensor_count] = (sensor_t){handler, div};
  _sensors.sensors_lookup[id] = &_sensors.sensors[_sensors.sensor_count++];
  return 0;
}

int register_sensor_ratings(sensor_id_t id, system_state_t state, int rating,
                            obd_code_t obd_code, uint8_t obd_fault,
                            uint8_t is_minimum, uint8_t required_samples,
                            int pool_ms)
{
  if(_sensors.rating_count >= MAX_SENSOR_RATINGS) return 1;
  _sensors.ratings[_sensors.rating_count++] = (sensor_ratings_t){
      id,        state,  rating, obd_code, is_minimum, required_samples,
      obd_fault, pool_ms};
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
