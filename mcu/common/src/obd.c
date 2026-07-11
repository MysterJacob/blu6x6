#include "obd.h"

#include <stdio.h>
#include <string.h>

#include "fail.h"
#include "port.h"
#include "storage.h"
#include "system.h"

static obd_fault_cfg_t _obd_faults[_OBD_FAULT_COUNT];

static int _list_faults_handler(int argc, char **argv)
{
  if(argc == 1) {
    puts("| 0.list 1.history");
    return -1;
  }

  if(strcmp(argv[1], "list") == 0) {
    for(int i = 0; i < _OBD_FAULT_COUNT; i++) {
      obd_fault_cfg_t obd_fault = _obd_faults[i];
      printf("%-8d %-8d %-15lu %-10d %-10x\n", obd_fault.id, obd_fault.status,
             obd_fault.last_bootcycle_present, obd_fault.flags,
             obd_fault.fault);
    }

  } else if(strcmp(argv[1], "history") == 0) {
    obd_fault_nv_t faults_b[32];
    size_t count;
    read_obd_faults_history(sizeof(faults_b) / sizeof(obd_fault_nv_t), &count,
                            faults_b);

    printf("printing %lu faults\n", count);
    for(size_t i = 0; i < count; i++) {
      obd_fault_nv_t fault = faults_b[i];
      printf("%-8d %-8d %-15lu %-10d\n", fault.id, fault.status,
             fault.last_bootcycle_present, fault.fault);
    }

  } else {
    return -1;
  }
  return 0;
}

void reset_old_faults()
{
  for(size_t i = 0; i < _OBD_FAULT_COUNT; i++) {
    if((_obd_faults[i].flags & OBD_AUTOCLEAR) == 0) continue;
    obd_clear(_obd_faults[i].id);
  }
}

int obd_init()
{
  ON_ERROR_ABORT(port_register_command("faults", _list_faults_handler));
  ON_ERROR_ABORT(read_obd_faults(_OBD_FAULT_COUNT, NULL, _obd_faults));
  reset_old_faults();
  return 0;
}

int obd_clear(obd_code_t code)
{
  if(code >= _OBD_FAULT_COUNT) return -1;
  obd_fault_cfg_t obd_fault = _obd_faults[code];
  if((obd_fault.flags & (OBD_HARDFAULT | OBD_PERSISTENT)) != 0) return -1;
  obd_forceclear(code);
  return 0;
}

int obd_forceclear(obd_code_t code)
{
  if(code >= _OBD_FAULT_COUNT) return -1;
  _obd_faults[code].status &= ~OBD_ACTIVE;
  obd_fault_cfg_t obd_fault = _obd_faults[code];
  store_obd_fault(code, obd_fault.status, obd_fault.fault,
                  obd_fault.last_bootcycle_present);
  return 0;
}

int obd_fault(obd_code_t code, uint8_t fault)
{
  if(code >= _OBD_FAULT_COUNT) return -1;
  _obd_faults[code].status = OBD_ACTIVE;
  obd_fault_cfg_t obd_fault = _obd_faults[code];

  if((obd_fault.flags & OBD_FAULT_OR) != 0) {
    _obd_faults[code].fault |= fault;
  } else if((obd_fault.flags & OBD_FAULT_ADD) != 0) {
    _obd_faults[code].fault += fault;
  }
  _obd_faults[code].last_bootcycle_present = sys.bootcycle;

  obd_fault = _obd_faults[code];
  store_obd_fault(code, obd_fault.status, obd_fault.fault,
                  obd_fault.last_bootcycle_present);

  if((obd_fault.flags & OBD_HARDFAULT) != 0) {
    __abort("OBD Hard Fault", code);
  }
  return 0;
}

int obd_register(const obd_code_t code, const obd_flags_t flags)
{
  if(code >= _OBD_FAULT_COUNT) return -1;
  _obd_faults[code].id = code;
  _obd_faults[code].flags = flags;
  _obd_faults[code].status = 0;
  _obd_faults[code].fault = 0;
  _obd_faults[code].last_bootcycle_present = 0;
  return 0;
}
