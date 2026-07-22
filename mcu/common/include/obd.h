#pragma once
#include <stdint.h>
typedef enum {
  OBD_SOFTFAULT = 0,
  OBD_HARDFAULT = 1,
  OBD_PERSISTENT = 2,
  OBD_AUTOCLEAR = 4,
  OBD_FAULT_ADD = 8,
  OBD_FAULT_OR = 16,
} obd_flags_t;
typedef enum { OBD_CLEAR = 0, OBD_ACTIVE = 2 } obd_status_t;
typedef enum {
  OBDC_TEST_SOFT,
  OBDC_TEST_HARD,
  BATTERY_LOW,
  BATTERY_CRITICAL,
  MOTOR_0_OVERCURRENT,
  MOTOR_0_OVERHEAT,
  MOTOR_0_POWERLOSS,
  DRIVER_0_OVERHEAT,
  MOTOR_0_FAULT,
  MOTOR_1_OVERCURRENT,
  MOTOR_1_OVERHEAT,
  MOTOR_1_POWERLOSS,
  DRIVER_1_OVERHEAT,
  MOTOR_1_FAULT,
  MOTOR_2_OVERCURRENT,
  MOTOR_2_OVERHEAT,
  MOTOR_2_POWERLOSS,
  DRIVER_2_OVERHEAT,
  MOTOR_2_FAULT,
  MOTOR_3_OVERCURRENT,
  MOTOR_3_OVERHEAT,
  MOTOR_3_POWERLOSS,
  DRIVER_3_OVERHEAT,
  MOTOR_3_FAULT,
  _OBD_FAULT_COUNT
} obd_code_t;

typedef struct _obd_error {
  obd_code_t id;
  obd_flags_t flags;
  obd_status_t status;
  uint8_t qualifier;
  uint32_t last_bootcycle_present;
} obd_fault_cfg_t;

int obd_init();
int obd_clear(obd_code_t code);
int obd_forceclear(obd_code_t code);
int obd_fault(obd_code_t code, uint8_t qualifier);
int obd_register(const obd_code_t code, const obd_flags_t flags);
int obd_active_fault_count();
int obd_setup();
