#include <avr/io.h>
#include <stdio.h>

#define BAUD 9600
#define UBRR_VAL (F_CPU / (16UL * BAUD) - 1)

static int serial_putc(char c, FILE *f __attribute__((unused)))
{
  while(!(UCSR0A & (1 << UDRE0)))
    ;
  UDR0 = c;
  if(c == '\n') return serial_putc('\r', f);
  return 0;
}

static int serial_getc(FILE *f __attribute__((unused)))
{
  while(!(UCSR0A & (1 << RXC0)))
    ;
  char c = UDR0;
  if(c == '\r') c = '\n';
  return (int)(unsigned char)c;
}

static FILE uart_stream =
    FDEV_SETUP_STREAM(serial_putc, serial_getc, _FDEV_SETUP_RW);

void port_setup_serial(void)
{
  UBRR0H = (uint8_t)(UBRR_VAL >> 8);
  UBRR0L = (uint8_t)(UBRR_VAL & 0xFF);
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  stdout = &uart_stream;
  stdin = &uart_stream;
}
