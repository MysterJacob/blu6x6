#pragma once
#include <stdint.h>

typedef struct _sys {
  uint64_t bootcycle;
} sys_t;

extern sys_t sys;
uint64_t get_ms_from_boot();
