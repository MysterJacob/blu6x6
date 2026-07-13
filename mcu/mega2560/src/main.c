#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "fail.h"
#include "obd.h"
#include "port.h"
#include "port_io.h"
#include "storage.h"
#include "system.h"

uint64_t boottime = 0;
sys_t sys = {0};

system_state_t init_h(state_change_params_t params);
system_state_t post_h(state_change_params_t params);
system_state_t idle_h(state_change_params_t params);
system_state_t drive_h(state_change_params_t params);

system_state_handler_t system_state_handlers[] = {init_h, post_h, idle_h,
                                                  drive_h};
system_reboot_reason_t get_reboot_reason()
{
  return POWERON;
}

static int obd_test(int argc, char **argv)
{
  if(argc == 1) {
    obd_fault(OBDC_TEST_SOFT, 0);
    return 0;
  }
  ON_ERROR_LOG(obd_fault(atoi(argv[1]), 0));
  return 0;
}

system_state_t init_h(state_change_params_t params)
{
  puts("Init");
  port_setup_serial();
  obd_register(OBDC_TEST_HARD, OBD_HARDFAULT);
  obd_register(OBDC_TEST_SOFT, OBD_SOFTFAULT);
  port_register_command("t", obd_test);
  return POST;
}

system_state_t post_h(state_change_params_t params)
{
  puts("Post");
  return IDLE;
}

system_state_t idle_h(state_change_params_t params)
{
  puts("Idle");
  char cmd[48];

  int code;
  while(1) {
    puts(">>>");
    fflush(stdout);

    if(fgets(cmd, sizeof(cmd), stdin) == NULL) continue;

    cmd[strcspn(cmd, "\r\n")] = '\0';

    if(cmd[0] == '\0') continue;

    code = port_process_command(cmd);
    printf("command exited with code: %d\n", code);
  }
}

system_state_t drive_h(state_change_params_t params)
{
  puts("Drive");
  return IDLE;
}

uint64_t get_ms_from_boot()
{
  return 0;
}
