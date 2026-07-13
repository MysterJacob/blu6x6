#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "fail.h"
#include "obd.h"
#include "port.h"
#include "sensors.h"
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

int mock_sensor()
{
  perform_sensors_obd();
  return 400 - 15 * get_ms_from_boot();
}

void *obd_thread_worker(void *params)
{
  while(1) {
    perform_sensors_obd();
  }
}

system_state_t init_h(__attribute__((unused)) state_change_params_t params)
{
  puts("Init");
  boottime = time(NULL);

  obd_register(OBDC_TEST_HARD, OBD_HARDFAULT);
  obd_register(OBDC_TEST_SOFT, OBD_SOFTFAULT);
  port_register_command("t", obd_test);
  register_sensor(MOTOR_1_CURRENT, mock_sensor, 1000);
  register_sensor_ratings(MOTOR_1_CURRENT, ANY, 0.3, OBDC_TEST_HARD, 1, 1, 2,
                          1);
  pthread_t obd_thread;
  pthread_create(&obd_thread, NULL, obd_thread_worker, NULL);
  return POST;
}

system_state_t post_h(__attribute__((unused)) state_change_params_t params)

{
  puts("Post");
  return IDLE;
}

system_state_t idle_h(__attribute__((unused)) state_change_params_t params)
{
  puts("Idle");
  char cmd[48];
  int code;
  int conv;
  while(1) {
    printf(">>>");
    fflush(stdout);
    while((conv = scanf("%47[^\n]", cmd)) == 0)
      ;
    getchar();
    if(conv < 0) return -1;
    code = port_process_command(cmd);
    printf("command exited with code: %d\n", code);
  }
  return IDLE;
}

system_state_t drive_h(__attribute__((unused)) state_change_params_t params)
{
  puts("Drive");
  return IDLE;
}

uint64_t get_ms_from_boot()
{
  return time(NULL) - boottime;
}
