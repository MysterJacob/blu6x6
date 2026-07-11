#include <stddef.h>
#include <stdint.h>

#include "obd.h"

#define MAX_ACTIVE_FAULTS 128

typedef struct __attribute__((packed)) {
  uint16_t id;
  uint8_t status;
  uint8_t fault;
  uint64_t last_bootcycle_present;
} obd_fault_nv_t;

typedef struct __attribute__((packed)) {
  uint16_t magic;
  uint16_t version;
  uint16_t count;
  uint16_t crc;
  obd_fault_nv_t active[MAX_ACTIVE_FAULTS];
} fault_store_t;

void init_storage();
int store_obd_faults(size_t buffer_size, const obd_fault_cfg_t faults[]);
int read_obd_faults(size_t buffer_size, size_t *count,
                    obd_fault_cfg_t faults[]);
