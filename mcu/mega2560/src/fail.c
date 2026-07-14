#include "fail.h"

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <stdio.h>

#include "signalisation.h"

void __attribute__((noreturn)) __abort(const char *line, int code)
{
  cli();

//   puts("\n!!ERROR ABORT!!");
//   printf("%s (ecode: %d)\n", line, code);
  signal_hardfault();

  while(1) {
    __asm("nop");
  }
}
void __log_error(const char *line, int code)
{
  printf("err: %s (ecode: %d)\n", line, code);
}
