#include "system.h"

#include "fail.h"
#include "obd.h"
#include "sensors.h"
#include "port.h"
#include "storage.h"

static struct {
  system_state_t state;
  system_state_t last_state;
} system_state;

void _init()
{
  system_state.state = INIT;
  ON_ERROR_ABORT(storage_init());
  ON_ERROR_ABORT(port_init());
  ON_ERROR_ABORT(obd_init());
  ON_ERROR_ABORT(sensors_init());
}

int main(void)
{
  system_reboot_reason_t reboot_reason = get_reboot_reason();
  switch(reboot_reason) {
    case POWERON:
      _init();
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
    system_state.last_state = system_state.state;
    system_state.state =
        handler((state_change_params_t){system_state.last_state, 0});
  };
}
