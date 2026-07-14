#include "system.h"

#include <avr/delay.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "avr/interrupt.h"
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
  TCCR2A = _BV(WGM21);
  TCCR2B = _BV(CS22);
  TCNT2 = 6;
  OCR2A = 249;
  TIMSK2 = _BV(OCIE2A);

  sei();
  return 0;
}

ISR(TIMER2_COMPA_vect)
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
  obd_register(OBDC_TEST_HARD, OBD_HARDFAULT);
  obd_register(OBDC_TEST_SOFT, OBD_SOFTFAULT | OBD_PERSISTENT);
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
