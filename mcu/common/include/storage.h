#pragma once

#include <stddef.h>
#include <stdint.h>

#include "obd.h"

#define MAX_HISTORY_FAULTS 128

typedef struct __attribute__((packed)) {
  uint16_t id;
  uint8_t status;
  uint8_t qualifier;
  uint32_t last_bootcycle_present;
} obd_fault_nv_t;

int storage_init(void);

int store_obd_fault(const uint16_t id, const uint8_t status,
                    const uint8_t qualifier, const uint64_t last_bootcycle_present);

int read_obd_faults(size_t buffer_size, size_t *active_count,
                    obd_fault_cfg_t faults[]);

int read_obd_faults_history(size_t buffer_size, size_t *count,
                            obd_fault_nv_t faults[]);
