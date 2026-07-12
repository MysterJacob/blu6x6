#include "obd.h"
#include "system.h"

#define MAX_SENSOR_RATINGS 128

typedef enum {
  BATTERY_VOLTAGE,
  MCU_VOLTAGE,
  MOTOR_1_POWER,
  MOTOR_1_CURRENT,
  MOTOR_2_POWER,
  MOTOR_2_CURRENT,
  MOTOR_3_POWER,
  MOTOR_3_CURRENT,
  MOTOR_4_POWER,
  MOTOR_4_CURRENT,
  MOTOR_5_POWER,
  MOTOR_5_CURRENT,
  _TOTAL_SENSOR_COUNT,
} sensor_id_t;

typedef int (*sensor_handler_t)(void);

typedef struct {
  sensor_handler_t handler;
  int div;
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
int register_sensor(sensor_id_t id, sensor_handler_t handler, int div);
int register_sensor_ratings(sensor_id_t id, system_state_t state, int rating,
                            obd_code_t obd_code, uint8_t obd_fault, uint8_t is_minimum,
                            uint8_t required_samples, int pool_ms);
int get_sensor_reading(sensor_id_t id, float *out);
int get_sensor_reading_raw(sensor_id_t id, int *out);

void perform_sensors_obd();
