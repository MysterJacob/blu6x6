#include "port_io.h"

#include <avr/io.h>
#include <stdio.h>

#define UBRR_VAL (F_CPU / (16UL * BAUD) - 1)

static char line_buf[LINE_BUF_SIZE];
static uint8_t line_len = 0;

static int serial_putc(char c, FILE *f __attribute__((unused)))
{
  while(!(UCSR0A & (1 << UDRE0)));
  UDR0 = c;
  if(c == '\n') return serial_putc('\r', f);
  return 0;
}

static int serial_getc_nb(void)
{
  if(!(UCSR0A & (1 << RXC0))) return -1;  // no data
  unsigned char c = UDR0;
  if(c == '\r') c = '\n';
  return (int)c;
}

char *uart_readline(void)
{
  int c;

  while((c = serial_getc_nb()) >= 0) {
    if(c == '\n') {
      line_buf[line_len] = '\0';
      line_len = 0;
      return line_buf;
    } else if(line_len < LINE_BUF_SIZE - 1) {
      line_buf[line_len++] = (char)c;
    } else {
    }
  }

  return NULL;
}

static FILE uart_stream =
    FDEV_SETUP_STREAM(serial_putc, NULL, _FDEV_SETUP_WRITE);

void port_setup_serial(void)
{
  UBRR0H = (uint8_t)(UBRR_VAL >> 8);
  UBRR0L = (uint8_t)(UBRR_VAL & 0xFF);
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  stdout = &uart_stream;
  stdin = &uart_stream;
}
