int signalisation_init();
typedef enum {
  SIG_OFF,
  SIG_ON,
  SIG_BLINK_NORMAL,
  SIG_BLINK_RAPID,
} sig_t;
typedef enum { RED, YELLOW, GREEN, BUZZER, _SIG_COLOR_COUNT } sig_color_t;
int set_signalization(sig_color_t color, sig_t t);
void update_signalization();
void signal_hardfault();
