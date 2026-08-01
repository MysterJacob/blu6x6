#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fail.h"
#include "motors.h"
#include "obd.h"
#include "port.h"
#include "port_io.h"
#include "rpi_port.h"
#include "signalisation.h"
#include "storage.h"
#include "system.h"

system_state_t init_h(state_change_params_t params);
system_state_t idle_h(state_change_params_t params);
system_state_t arming_h(state_change_params_t params);
system_state_t armed_h(state_change_params_t params);

system_state_handler_t system_state_handlers[] = {init_h, idle_h, arming_h,
                                                  armed_h};
struct {
  float rhs;
  float lhs;
  int arm;
} io_port;

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

static int port_arm(__attribute__((unused)) int argc,
                    __attribute__((unused)) char **argv)
{
  io_port.arm = 1;
  return 0;
}

static int port_motor(__attribute__((unused)) int argc,
                      __attribute__((unused)) char **argv)
{
  if(argc == 3) {
    io_port.lhs = atoi(argv[1]);
    io_port.rhs = atoi(argv[2]);
  } else {
    io_port.rhs = 0;
    io_port.lhs = 0;
    io_port.arm = 0;
  }
  return 0;
}

system_state_t init_h(__attribute__((unused)) state_change_params_t params)
{
  port_setup_serial();

  port_register_command("t", obd_test);
  port_register_command("time", debug_sys_time);
  port_register_command("arm", port_arm);
  port_register_command("motor", port_motor);
  printf("boot #%lu\n", sys.bootcycle);
  printf("boot flags %lu\n", sys.bootflags);
  puts("P.O.S.T. done");
  set_signalization(GREEN, SIG_ON);
  fflush(stdout);
  return IDLE;
}

void process_port_comm()
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
}

system_state_t idle_h(__attribute__((unused)) state_change_params_t params)
{
  if(params.last_state != IDLE) {
    set_drive(0, 0);
  }
  process_port_comm();
  if(io_port.arm == 1 || rpi_port.arm == 1) return ARMING;
  return IDLE;
}

int32_t arm_timestamp;
system_state_t arming_h(__attribute__((unused)) state_change_params_t params)
{
  int32_t time = get_ms_from_boot();
  if(params.last_state != ARMING) arm_timestamp = time;
  if(time - arm_timestamp) return ARMED;
  io_port.rhs = 0;
  io_port.lhs = 0;
  return ARMING;
}

system_state_t armed_h(__attribute__((unused)) state_change_params_t params)
{
  if(io_port.arm == 0 && rpi_port.arm == 0) return IDLE;
  if(io_port.arm) {
    set_drive(io_port.lhs, io_port.rhs);
  } else if(rpi_port.arm) {
    set_drive(rpi_port.lhs, rpi_port.rhs);
  }
  return ARMED;
}
