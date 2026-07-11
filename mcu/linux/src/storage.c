#include <stddef.h>

#include "obd.h"
#include "stdint.h"

static struct {
  size_t fault_count;
  obd_fault_cfg_t faults[64];
} storage = {0};

void init_storage()
{
}
int store_obd_faults(size_t buffer_size, const obd_fault_cfg_t faults[])
{
  if(buffer_size > 64) return -1;
  for(size_t i = 0; i < buffer_size; i++) {
    storage.faults[i] = faults[i];
  }
  storage.fault_count = buffer_size;
  return 0;
}
int read_obd_faults(size_t buffer_size, size_t *count, obd_fault_cfg_t faults[])
{
  const size_t read_count =
      buffer_size > storage.fault_count ? buffer_size : storage.fault_count;
  size_t i = 0;
  for(; i < read_count; i++) {
    faults[i] = storage.faults[i];
  }
  *count = i;
  return 0;
}
