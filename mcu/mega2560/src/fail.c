#include "fail.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "motors.h"
#include "signalisation.h"

static uint8_t panic_stack[128];
void __attribute__((noreturn)) __abort(const char *line, int code)
{
  cli();
  signal_hardfault();
  motors_estop();

  puts("ABORT");
  printf("err: %s (ecode: %d)\n", line, code);

  while(1) {
    __asm("nop");
  }
}

void __log_error(const char *line, int code)
{
  printf("err: %s (ecode: %d)\n", line, code);
}
