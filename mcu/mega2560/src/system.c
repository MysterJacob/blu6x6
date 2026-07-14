#include "system.h"

#include <avr/delay.h>
#include <avr/eeprom.h>
#include <stdint.h>

#include "avr/interrupt.h"
#include "obd.h"

static uint32_t EEMEM ee_boot_cycles;

sys_t sys = {0};

uint32_t boot_cycles_increment(void);
int system_init()
{
  _delay_ms(1000);
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
  uint32_t count = eeprom_read_dword(&ee_boot_cycles);
  count++;
  eeprom_update_dword(&ee_boot_cycles, count);
  return count;
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
