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
typedef enum { OBD_TEST, _OBD_GUARD_LAST } obd_fault_t;

typedef struct _obd_error {
  obd_fault_t code;
  obd_flags_t flags;
  obd_status_t status;
  uint8_t fault;
  uint64_t last_bootcycle_present;
} obd_fault_cfg_t;

int obd_init();
int obd_clear(obd_fault_t code);
int obd_forceclear(obd_fault_t code);
int obd_fault(obd_fault_t code, uint8_t fault);
int obd_register(const obd_fault_t code, const obd_flags_t flags);
