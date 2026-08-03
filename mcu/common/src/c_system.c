#include <stdio.h>
#include "fail.h"
#include "motors.h"
#include "obd.h"
#include "port.h"
#include "rpi_port.h"
#include "sensors.h"
#include "signalisation.h"
#include "storage.h"
#include "system.h"

static struct {
  system_state_t state;
  system_state_t last_state;
} system_state;

void _mcu_init()
{
  system_state.state = INIT;
  ON_ERROR_ABORT(system_init());
  ON_ERROR_ABORT(signalisation_init());
  ON_ERROR_ABORT(port_init());
  ON_ERROR_ABORT(storage_init());
  ON_ERROR_ABORT(obd_init());
  ON_ERROR_ABORT(sensors_init());
  ON_ERROR_ABORT(sensors_setup());
  ON_ERROR_ABORT(motors_init());
}

void update_sig_loop()
{
  if(obd_active_fault_count() > 5)
    set_signalization(YELLOW, SIG_BLINK_RAPID);
  else if(obd_active_fault_count() > 0)
    set_signalization(YELLOW, SIG_BLINK_NORMAL);
  else
    set_signalization(YELLOW, SIG_OFF);
}
int main(void)
{
  system_reboot_reason_t reboot_reason = get_reboot_reason();
  switch(reboot_reason) {
    case POWERON:
      _mcu_init();
      break;
    case BROWNOUT:
      ABORT(REBOOT_FAIL);
      break;
    case EXTERN:
      ABORT(REBOOT_FAIL);
      break;
    case CRASH:
      ABORT(REBOOT_FAIL);
      break;
  }
  while(1) {
    system_state_handler_t handler = system_state_handlers[system_state.state];
    if(handler == 0) ABORT(SATE_SWITCH_FAIL);

    system_state_t previous_state = system_state.state;
    system_state.state =
        handler((state_change_params_t){system_state.last_state, 0});
    system_state.last_state = previous_state;
    if(system_state.state != system_state.last_state) {
      printf("Switched from %d to %d\n>>>", system_state.last_state,
             system_state.state);
    }

    perform_sensors_obd();

    rpi_port_update();
    update_sig_loop();
    update_signalization();
  };
}
system_state_t get_system_state()
{
  return system_state.state;
}
