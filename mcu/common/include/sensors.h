#include "obd.h"
#include "system.h"

#define MAX_SENSOR_RATINGS 128

typedef enum {
  BATTERY_VOLTAGE,
  MCU_VOLTAGE,
  MOTOR_0_POWER,
  DRIVER_0_TEMPERATURE,
  MOTOR_0_TEMPERATURE,
  MOTOR_0_CURRENT,
  MOTOR_1_POWER,
  DRIVER_1_TEMPERATURE,
  MOTOR_1_TEMPERATURE,
  MOTOR_1_CURRENT,
  MOTOR_2_POWER,
  DRIVER_2_TEMPERATURE,
  MOTOR_2_TEMPERATURE,
  MOTOR_2_CURRENT,
  MOTOR_3_POWER,
  DRIVER_3_TEMPERATURE,
  MOTOR_3_TEMPERATURE,
  MOTOR_3_CURRENT,
  _TOTAL_SENSOR_COUNT,
} sensor_id_t;

const char *sensor_id_to_string(sensor_id_t id);
typedef int (*sensor_handler_t)(sensor_id_t id);

typedef struct {
  sensor_id_t id;
  sensor_handler_t handler;
  int div;
  char unit;
} sensor_t;

typedef struct {
  sensor_id_t id;
  system_state_t system_state;
  float rating;
  obd_code_t obd_code;
  uint8_t obd_fault;
  uint8_t is_minimum;
  uint8_t required_samples;
  uint32_t pool_ms;
} sensor_ratings_t;

int sensors_init();
int sensors_setup();
int register_sensor(sensor_id_t id, sensor_handler_t handler, int div,
                    char unit);
int register_sensor_ratings(sensor_id_t id, system_state_t state, float rating,
                            obd_code_t obd_code, uint8_t obd_fault,
                            uint8_t is_minimum, uint8_t required_samples,
                            int pool_ms);
int get_sensor_reading(sensor_id_t id, float *out);
int get_sensor_reading_raw(sensor_id_t id, int *out);

void perform_sensors_obd();
