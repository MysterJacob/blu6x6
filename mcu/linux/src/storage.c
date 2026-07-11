#include "storage.h"

#include <stddef.h>

#include "obd.h"
#include "stdint.h"

static fault_store_t storage;

void init_storage()
{
  storage.magic = 0xCAFE;
  storage.version = 0xCAFE;
  storage.crc = 0xCAFE;
  storage.count = 0;
}

int store_obd_faults(const obd_fault_cfg_t faults[])
{
  size_t current_fault = 0;
  for(size_t i = 0; i < current_fault && current_fault < MAX_ACTIVE_FAULTS;
      i++) {
    if((faults[i].status & OBD_ACTIVE) == 0) continue;
    const obd_fault_cfg_t fault = faults[i];
    storage.active[current_fault++] = (obd_fault_nv_t){
        fault.id, fault.status, fault.fault, fault.last_bootcycle_present};
  }
  storage.count = current_fault;
  return 0;
}

int read_obd_faults(size_t buffer_size, size_t *active_count,
                    obd_fault_cfg_t faults[])
{
  for(size_t i = 0; i < storage.count; i++) {
    for(size_t j = 0; j < buffer_size; j++) {
      const obd_fault_nv_t fault = storage.active[i];
      if(fault.id != faults[j].id) continue;
      faults[j].last_bootcycle_present = fault.last_bootcycle_present;
      faults[j].fault = fault.fault;
      faults[j].status = fault.status;
      (*active_count)++;
    }
  }
  return 0;
}
