#include <string.h>

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

int sensors_init()
{
  memset(&_sensors, 0, sizeof(_sensors));
  memset(_sensors_obd, 0, sizeof(_sensors_obd));
  return 0;
}

int register_sensor(sensor_id_t id, sensor_handler_t handler, int div)
{
  if(_sensors.sensor_count >= _TOTAL_SENSOR_COUNT) return 1;
  if(_sensors.sensors_lookup[id] != 0) return 2;
  if(div == 0) return 3;

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
