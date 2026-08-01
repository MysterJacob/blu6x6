#include "system.h"

#include <avr/delay.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "avr/interrupt.h"
#include "fail.h"
#include "obd.h"

#define ODB_RESET_PIN PL0

static uint32_t EEMEM ee_boot_cycles;

sys_t sys = {0};

uint32_t boot_cycles_increment(void);
void set_boot_flags();
int system_init()
{
  _delay_ms(1000);
  set_boot_flags();
  sys.bootcycle = boot_cycles_increment();
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS00) | _BV(CS01);
  OCR0A = 249;
  TIMSK0 = _BV(OCIE0A);

  sei();
  return 0;
}

ISR(TIMER0_COMPA_vect)
{
  sys.ms_from_boot += 1;
}

uint32_t boot_cycles_increment(void)
{
  uint32_t boot_count = eeprom_read_dword(&ee_boot_cycles);
  boot_count++;
  eeprom_update_dword(&ee_boot_cycles, boot_count);
  return boot_count;
}

uint64_t get_ms_from_boot()
{
  cli();
  uint64_t time = sys.ms_from_boot;
  sei();
  return time;
}

// FIXME UPGRADE TO OPTIBOOT
system_reboot_reason_t get_reboot_reason()
{
  return POWERON;
}

int obd_setup()
{
  ON_ERROR_ABORT(obd_register(OBDC_TEST_HARD, OBD_HARDFAULT));
  ON_ERROR_ABORT(obd_register(OBDC_TEST_SOFT, OBD_SOFTFAULT | OBD_PERSISTENT));

  ON_ERROR_ABORT(obd_register(BATTERY_LOW, OBD_SOFTFAULT));
  ON_ERROR_ABORT(obd_register(BATTERY_CRITICAL, OBD_HARDFAULT));

  ON_ERROR_ABORT(obd_register(
      MOTOR_0_OVERCURRENT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_0_OVERHEAT, OBD_SOFTFAULT | OBD_PERSISTENT |
                                                    OBD_AUTOCLEAR |
                                                    OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(
      DRIVER_0_OVERHEAT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(
      obd_register(MOTOR_0_POWERLOSS, OBD_HARDFAULT | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_0_FAULT, OBD_HARDFAULT | OBD_QUALIFIER_OR));

  ON_ERROR_ABORT(obd_register(
      MOTOR_1_OVERCURRENT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_1_OVERHEAT, OBD_SOFTFAULT | OBD_PERSISTENT |
                                                    OBD_AUTOCLEAR |
                                                    OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(
      DRIVER_1_OVERHEAT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(
      obd_register(MOTOR_1_POWERLOSS, OBD_HARDFAULT | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_1_FAULT, OBD_HARDFAULT | OBD_QUALIFIER_OR));

  ON_ERROR_ABORT(obd_register(
      MOTOR_2_OVERCURRENT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_2_OVERHEAT, OBD_SOFTFAULT | OBD_PERSISTENT |
                                                    OBD_AUTOCLEAR |
                                                    OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(
      DRIVER_2_OVERHEAT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(
      obd_register(MOTOR_2_POWERLOSS, OBD_HARDFAULT | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_2_FAULT, OBD_HARDFAULT | OBD_QUALIFIER_OR));

  ON_ERROR_ABORT(obd_register(
      MOTOR_3_OVERCURRENT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_3_OVERHEAT, OBD_SOFTFAULT | OBD_PERSISTENT |
                                                    OBD_AUTOCLEAR |
                                                    OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(
      DRIVER_3_OVERHEAT,
      OBD_SOFTFAULT | OBD_PERSISTENT | OBD_AUTOCLEAR | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(
      obd_register(MOTOR_3_POWERLOSS, OBD_HARDFAULT | OBD_QUALIFIER_OR));
  ON_ERROR_ABORT(obd_register(MOTOR_3_FAULT, OBD_HARDFAULT | OBD_QUALIFIER_OR));

  return 0;
}

void set_boot_flags()
{
  DDRL &= ~_BV(ODB_RESET_PIN);
  PORTL |= _BV(ODB_RESET_PIN);
  _delay_ms(50);

  int status = 0;
  for(uint16_t i = 0; i <= 10000; i++) {
    status |= PINL & _BV(ODB_RESET_PIN);
    _delay_ms(1);
    if(status != 0) break;
  }
  if((status & _BV(ODB_RESET_PIN)) == 0) sys.bootflags |= SYS_RESTART_ODB;
}
