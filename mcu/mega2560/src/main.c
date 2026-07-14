#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fail.h"
#include "motors.h"
#include "obd.h"
#include "port.h"
#include "port_io.h"
#include "signalisation.h"
#include "storage.h"
#include "system.h"

system_state_t init_h(state_change_params_t params);
system_state_t post_h(state_change_params_t params);
system_state_t idle_h(state_change_params_t params);
system_state_t drive_h(state_change_params_t params);

system_state_handler_t system_state_handlers[] = {init_h, post_h, idle_h,
                                                  drive_h};

static int obd_test(int argc, char **argv)
{
  if(argc == 1) {
    obd_fault(OBDC_TEST_SOFT, 0);
    return 0;
  }
  ON_ERROR_LOG(obd_fault(atoi(argv[1]), 0));
  return 0;
}

static int debug_sys_time(__attribute__((unused)) int argc,
                          __attribute__((unused)) char **argv)
{
  printf("Time from boot: %lums\n", (uint32_t)get_ms_from_boot());
  fflush(stdout);
  return 0;
}

static int port_motor(__attribute__((unused)) int argc,
                      __attribute__((unused)) char **argv)
{
  if(argc != 3) set_drive(0, 0);
  set_drive(atoi(argv[1]), atoi(argv[2]));
  return 0;
}

system_state_t init_h(__attribute__((unused)) state_change_params_t params)
{
  port_setup_serial();

  port_register_command("t", obd_test);
  port_register_command("time", debug_sys_time);
  port_register_command("motor", port_motor);
  return POST;
}

system_state_t post_h(__attribute__((unused)) state_change_params_t params)
{
  printf("boot #%lu\n", sys.bootcycle);
  printf("boot flags %lu\n", sys.bootflags);
  puts("P.O.S.T. done");
  set_signalization(GREEN, SIG_ON);
  fflush(stdout);
  return IDLE;
}

system_state_t idle_h(__attribute__((unused)) state_change_params_t params)
{
  static char cmd[48];
  static int code;

  char *line = uart_readline();
  if(line != NULL) {
    strncpy(cmd, line, sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    cmd[strcspn(cmd, "\r\n")] = '\0';

    if(cmd[0] != '\0') {
      printf("\n");
      code = port_process_command(cmd);
      printf("command exited with code: %d\n", code);
    }
    printf("\n>>>");
    fflush(stdout);
  }

  if(is_driving()) {
    set_signalization(GREEN, SIG_BLINK_NORMAL);
    set_signalization(BUZZER, SIG_BLINK_RAPID);
    return DRIVING;
  }
  return IDLE;
}

system_state_t drive_h(__attribute__((unused)) state_change_params_t params)
{
  if(!is_driving()) {
    set_signalization(GREEN, SIG_ON);
    set_signalization(BUZZER, SIG_OFF);
    return IDLE;
  }
  return DRIVING;
}
