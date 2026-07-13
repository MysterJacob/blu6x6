#include "fail.h"

#include <stdio.h>

void __attribute__((noreturn)) __abort(const char *line, int code)
{
  puts("!!ERROR ABORT!!");
  printf("%s (ecode: %d)\n", line, code);
  while(1) {
  }
}
void __log_error(const char *line, int code)
{
  printf("err: %s (ecode: %d)\n", line, code);
}
