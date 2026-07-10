#include <stdio.h>
#include <time.h>

#include "obd.h"
#include "port.h"
#include "system.h"

uint64_t boottime = 0;
sys_t sys = {0};
static int obd_test(int argc, char **argv)
{
  obd_fault(OBD_TEST, 0);
  return 0;
}
int main()
{
  boottime = time(NULL);
  port_init();
  obd_init();
  obd_register(OBD_TEST, OBD_HARDFAULT);
  port_register_command("t", obd_test);

  char cmd[48];
  int code;
  int conv;
  while(1) {
    printf(">>>");
    fflush(stdout);
    while((conv = scanf("%47s", cmd)) == 0) {
    };
    if(conv < 0) return -1;
    code = port_process_command(cmd);
    printf("command exited with code: %d\n", code);
  }
}

uint64_t get_ms_from_boot()
{
  return time(NULL) - boottime;
}
