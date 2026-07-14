#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "fail.h"
#include "obd.h"
#include "port.h"
#include "storage.h"
#include "system.h"

obd_fault_cfg_t obd_fault_table[_OBD_FAULT_COUNT];
static uint32_t fault_count = 0;

static void _recount_active_faults(void)
{
  fault_count = 0;
  for(size_t i = 0; i < _OBD_FAULT_COUNT; i++) {
    obd_fault_cfg_t *fault = &obd_fault_table[i];
    if((fault->status & OBD_ACTIVE) == 0) continue;

    fault_count++;
    if((fault->flags & OBD_HARDFAULT) != 0) {
      __abort("OBD Hard Fault", fault->id);
    }
  }
}

static int _list_faults_handler(int argc, char **argv)
{
  if(argc == 1) {
    puts("| 0.list 1.history");
    return -1;
  }

  if(strcmp(argv[1], "list") == 0) {
    puts("ID       STATUS   LBP             FLAGS      QUALIFIER");
    puts("-------- -------- --------------- ---------- ----------");
    for(size_t i = 0; i < _OBD_FAULT_COUNT; i++) {
      const obd_fault_cfg_t *obd_fault = &obd_fault_table[i];
      printf("%-8d %-8d %-15lu %-10d 0x%-8x\n", obd_fault->id,
             obd_fault->status, obd_fault->last_bootcycle_present,
             obd_fault->flags, obd_fault->qualifier);
    }
    puts("-------- -------- --------------- ---------- ----------");
  } else if(strcmp(argv[1], "history") == 0) {
    obd_fault_nv_t faults_b[32];
    size_t count = 0;

    read_obd_faults_history(sizeof(faults_b) / sizeof(faults_b[0]), &count,
                            faults_b);

    puts("ID       STATUS   LBP             FLAGS      QUALIFIER");
    puts("-------- -------- --------------- ---------- ----------");
    for(size_t i = 0; i < count; i++) {
      const obd_fault_nv_t *fault = &faults_b[i];
      printf("%-8d\t%-8d\t%-15lu\t%-10d :\n", fault->id, fault->status,
             fault->last_bootcycle_present, fault->qualifier);
    }
    puts("-------- -------- --------------- ---------- ----------");

  } else {
    return -1;
  }

  return 0;
}

void reset_old_faults(void)
{
  for(size_t i = 0; i < _OBD_FAULT_COUNT; i++) {
    obd_fault_cfg_t *fault = &obd_fault_table[i];

    if((fault->status & OBD_ACTIVE) == 0) continue;
    if((fault->flags & OBD_AUTOCLEAR) == 0) continue;

    if(sys.bootcycle >= (fault->last_bootcycle_present + 10U)) {
      obd_forceclear(fault->id);
    }
  }
}

void _boot_clear_faults()
{
  if((sys.bootflags & RESTART_ODB) == 0) return;
  for(size_t i = 0; i < _OBD_FAULT_COUNT; i++) {
    obd_fault_cfg_t *fault = &obd_fault_table[i];
    if((fault->status & OBD_ACTIVE) == 0) continue;
    obd_forceclear(fault->id);
  }
}

int obd_init(void)
{
  puts("obd init");
  ON_ERROR_ABORT(obd_setup());
  ON_ERROR_ABORT(read_obd_faults(_OBD_FAULT_COUNT, NULL, obd_fault_table));

  //   _boot_clear_faults();
  _recount_active_faults();
  reset_old_faults();
  _recount_active_faults();

  ON_ERROR_ABORT(port_register_command("faults", _list_faults_handler));

  return 0;
}

int obd_clear(obd_code_t code)
{
  if(code >= _OBD_FAULT_COUNT) return -1;

  const obd_fault_cfg_t *obd_fault = &obd_fault_table[code];

  if((obd_fault->flags & (OBD_HARDFAULT | OBD_PERSISTENT)) != 0) return -1;

  return obd_forceclear(code);
}

int obd_forceclear(obd_code_t code)
{
  if(code >= _OBD_FAULT_COUNT) return -1;

  obd_fault_cfg_t *obd_fault = &obd_fault_table[code];

  if((obd_fault->status & OBD_ACTIVE) != 0 && fault_count > 0) {
    fault_count--;
  }

  obd_fault->status &= ~OBD_ACTIVE;
  obd_fault->qualifier = 0;

  ON_ERROR_ABORT(store_obd_fault(code, obd_fault->status, obd_fault->qualifier,
                                 obd_fault->last_bootcycle_present));
  return 0;
}
int obd_fault(obd_code_t code, uint8_t fault)
{
  if(code >= _OBD_FAULT_COUNT) return -1;

  obd_fault_cfg_t *obd_fault = &obd_fault_table[code];
  const int was_active = ((obd_fault->status & OBD_ACTIVE) != 0);

  obd_fault->status |= OBD_ACTIVE;

  if((obd_fault->flags & OBD_FAULT_OR) != 0) {
    obd_fault->qualifier |= fault;
  } else if((obd_fault->flags & OBD_FAULT_ADD) != 0) {
    obd_fault->qualifier += fault;
  } else {
    obd_fault->qualifier = fault;
  }

  obd_fault->last_bootcycle_present = sys.bootcycle;

  if(!was_active) {
    fault_count++;
  }

  ON_ERROR_ABORT(store_obd_fault(code, obd_fault->status, obd_fault->qualifier,
                                 obd_fault->last_bootcycle_present));

  if((obd_fault->flags & OBD_HARDFAULT) != 0) {
    __abort("OBD Hard Fault", code);
  }

  return 0;
}

int obd_register(const obd_code_t code, const obd_flags_t flags)
{
  if(code >= _OBD_FAULT_COUNT) return -1;

  obd_fault_table[code].id = code;
  obd_fault_table[code].flags = flags;
  obd_fault_table[code].status = 0;
  obd_fault_table[code].qualifier = 0;
  obd_fault_table[code].last_bootcycle_present = 0;

  return 0;
}

int obd_active_fault_count(void)
{
  return (int)fault_count;
}
