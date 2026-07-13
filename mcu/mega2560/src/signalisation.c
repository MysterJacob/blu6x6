#include "signalisation.h"

#include <avr/io.h>
#include <string.h>

#include "system.h"

static sig_t sig_status[_SIG_COLOR_COUNT];
#define PORT_MASK (_BV(PA0) | _BV(PA1) | _BV(PA2) | _BV(PA3))

int init_signalization()
{
  for(int i = 0; i < _SIG_COLOR_COUNT; i++) {
    sig_status[i] = SIG_OFF;
  }
  DDRA |= PORT_MASK;
  update_signalization();
  return 0;
}
int set_signalization(sig_color_t color, sig_t t)
{
  sig_status[color] = t;
  return 0;
}
static inline int status_helper(sig_t t)
{
  switch(t) {
    case SIG_OFF:
      return 0;
    case SIG_ON:
      return 1;
    case SIG_BLINK_NORMAL:
      return (get_ms_from_boot() % 1000 < 500);
    case SIG_BLINK_RAPID:
      return (get_ms_from_boot() % 500 < 150);
    default:
      return 0;
  }
}

void update_signalization()
{
  PORTA |= PORT_MASK;
  if(status_helper(sig_status[RED])) PORTA &= ~_BV(PA0);
  if(status_helper(sig_status[YELLOW])) PORTA &= ~_BV(PA1);
  if(status_helper(sig_status[GREEN])) PORTA &= ~_BV(PA2);
  if(status_helper(sig_status[BUZZER])) PORTA &= ~_BV(PA3);
}
