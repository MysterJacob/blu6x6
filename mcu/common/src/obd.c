#include "obd.h"

#include <stdio.h>

#include "fail.h"
#include "port.h"
#include "system.h"

static obd_fault_cfg_t _obd_faults[_OBD_GUARD_LAST];

static int _list_faults_handler(int argc, char **argv)
{
  for(int i = 0; i < _OBD_GUARD_LAST; i++) {
    obd_fault_cfg_t obd_fault = _obd_faults[i];
    printf("%-8d %-8d %-15lu %-10d %-10x\n", obd_fault.code, obd_fault.status,
           obd_fault.last_bootcycle_present, obd_fault.flags, obd_fault.fault);
  }
  return 0;
}

int obd_init()
{
  ON_ERROR_ABORT(port_register_command("faults", _list_faults_handler));
  return 0;
}

int obd_clear(obd_fault_t code)
{
  if(code >= _OBD_GUARD_LAST) return -1;
  const obd_fault_cfg_t obd_fault = _obd_faults[code];
  if((obd_fault.status & (OBD_HARDFAULT | OBD_PERSISTENT)) != 0) return -1;
  _obd_faults[code].status &= ~OBD_ACTIVE;
  return 0;
}

int obd_forceclear(obd_fault_t code)
{
  if(code >= _OBD_GUARD_LAST) return -1;
  _obd_faults[code].status &= ~OBD_ACTIVE;
  return 0;
}

int obd_fault(obd_fault_t code, uint8_t fault)
{
  if(code >= _OBD_GUARD_LAST) return -1;
  _obd_faults[code].status = OBD_ACTIVE;
  const obd_fault_cfg_t obd_fault = _obd_faults[code];
  if((obd_fault.status & OBD_FAULT_OR) != 0) {
    _obd_faults[code].fault |= fault;
  } else if((obd_fault.status & OBD_FAULT_ADD) != 0) {
    _obd_faults[code].fault += fault;
  }
  _obd_faults[code].last_bootcycle_present = sys.bootcycle;
  if((obd_fault.flags & OBD_HARDFAULT) != 0) {
    __abort("OBD Hard Fault", code);
  }
  return 0;
}

int obd_register(const obd_fault_t code, const obd_flags_t flags)
{
  if(code >= _OBD_GUARD_LAST) return -1;
  _obd_faults[code].code = code;
  _obd_faults[code].flags = flags;
  _obd_faults[code].status = 0;
  _obd_faults[code].fault = 0;
  _obd_faults[code].last_bootcycle_present = 0;
  return 0;
}
