#pragma once
#include <stdint.h>

typedef enum { REBOOT_FAIL = 2001, SATE_SWITCH_FAIL = 2002 } sys_error;

typedef struct _sys {
  uint64_t bootcycle;
} sys_t;

extern sys_t sys;
uint64_t get_ms_from_boot();

typedef enum { POWERON, BROWNOUT, EXTERN, CRASH } system_reboot_reason_t;

system_reboot_reason_t get_reboot_reason();

typedef enum { ANY=-1, INIT = 0, POST, IDLE, DRIVING, _SYS_STATE_COUNT } system_state_t;

typedef struct {
  system_state_t last_state;
  void *params;
} state_change_params_t;

typedef system_state_t (*system_state_handler_t)(state_change_params_t params);

extern system_state_handler_t system_state_handlers[_SYS_STATE_COUNT];

system_state_t get_system_state();
