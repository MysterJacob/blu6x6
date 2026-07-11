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
typedef enum { OBD_TEST_SOFT, OBD_TEST_HARD, _OBD_FAULT_COUNT } obd_code_t;

typedef struct _obd_error {
  obd_code_t id;
  obd_flags_t flags;
  obd_status_t status;
  uint8_t fault;
  uint64_t last_bootcycle_present;
} obd_fault_cfg_t;

int obd_init();
int obd_clear(obd_code_t code);
int obd_forceclear(obd_code_t code);
int obd_fault(obd_code_t code, uint8_t fault);
int obd_register(const obd_code_t code, const obd_flags_t flags);
