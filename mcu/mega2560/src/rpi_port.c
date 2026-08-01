#include "rpi_port.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <util/atomic.h>

#include "proto.h"

rpi_port_t rpi_port;

#define RXBUF_SIZE 128
#define RXBUF_MASK (RXBUF_SIZE - 1)

static volatile uint8_t rxBuf[RXBUF_SIZE];
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static volatile uint8_t rxOverflow = 0;

static void uart2_init(uint32_t baud)
{
  uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;

  UBRR2H = (uint8_t)(ubrr >> 8);
  UBRR2L = (uint8_t)ubrr;

  UCSR2A = 0;
  UCSR2B = (1 << RXEN2) | (1 << TXEN2) | (1 << RXCIE2);
  UCSR2C = (1 << UCSZ21) | (1 << UCSZ20);
}

ISR(USART2_RX_vect)
{
  uint8_t data = UDR2;
  uint8_t nextHead = (rxHead + 1) & RXBUF_MASK;

  if(nextHead != rxTail) {
    rxBuf[rxHead] = data;
    rxHead = nextHead;
  } else {
    rxOverflow = 1;
  }
}

static uint8_t rxAvailable(void)
{
  uint8_t head, tail;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    head = rxHead;
    tail = rxTail;
  }
  return (uint8_t)((head - tail) & RXBUF_MASK);
}

static uint8_t rxRead(uint8_t *out)
{
  uint8_t tail;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
  {
    if(rxHead == rxTail) {
      return 0;
    }
    tail = rxTail;
    *out = rxBuf[tail];
    rxTail = (tail + 1) & RXBUF_MASK;
  }
  return 1;
}

static void uart2_putc(uint8_t c)
{
  while(!(UCSR2A & (1 << UDRE2))) {
  }
  UDR2 = c;
}

void packetHandler(const PacketHeader header, void *packetData)
{
  switch(header.id) {
    case arm_ID:
      rpi_port.arm = 1;
      break;
    case disarm_ID:
      rpi_port.arm = 0;
      break;
    case setDrive_ID: {
      const setDrive *packet = packetData;
      rpi_port.rhs = packet->rhs / 100.0f;
      rpi_port.lhs = packet->lhs / 100.0f;
    } break;
  }
}

void errorHandler(const protoErrorCode code)
{
}

int rpi_port_init(int baudrate)
{
  setPacketCallback(packetHandler);
  setErrorCallback(errorHandler);
  uart2_init(baudrate);
}

int rpi_port_update()
{
//   uint8_t out;
//   while(rxRead(&out)) {
//     puts("a");
//     processByte(out);
//   }
}
