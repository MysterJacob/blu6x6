#include "storage.h"

#include <stddef.h>

#include "obd.h"
#include "stdint.h"

static fault_present_store_t storage;
static fault_history_store_t history;

void storage_init()
{
  storage.magic = 0xCAFE;
  storage.version = 0xCAFE;
  storage.crc = 0xCAFE;
  storage.count = 0;
  history.magic = 0xCAFE;
  history.version = 0xCAFE;
  history.crc = 0xCAFE;
  history.count = 0;
}

int store_obd_fault(const uint16_t id, const uint8_t status,
                    const uint8_t fault, const uint64_t last_bootcycle_present)
{
  storage.faults[id] =
      (obd_fault_nv_t){id, status, fault, last_bootcycle_present};

  history.faults[history.count++ % MAX_HISTORY_FAULTS] =
      (obd_fault_nv_t){id, status, fault, last_bootcycle_present};
  return 0;
}

int read_obd_faults(size_t buffer_size, size_t *active_count,
                    obd_fault_cfg_t faults[])
{
  size_t fault_count = 0;
  for(size_t i = 0; i < storage.count; i++) {
    const obd_fault_nv_t fault = storage.faults[i];
    if(fault.id != faults[fault.id].id || buffer_size <= fault.id) continue;
    faults[fault.id].last_bootcycle_present = fault.last_bootcycle_present;
    faults[fault.id].fault = fault.fault;
    faults[fault.id].status = fault.status;
    fault_count++;
  }
  if(active_count != nullptr) *active_count = fault_count;
  return 0;
}

int read_obd_faults_history(size_t buffer_size, size_t *count,
                            obd_fault_nv_t *faults)
{
  size_t i = 0;
  for(; i < buffer_size && i < history.count; i++) {
    faults[i] = history.faults[(history.count - 1 - i + MAX_HISTORY_FAULTS) %
                               MAX_HISTORY_FAULTS];
  }
  *count = i;
  return 0;
}
