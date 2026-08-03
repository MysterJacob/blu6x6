#include "rpi_port.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/atomic.h>

#include "fail.h"
#include "system.h"

rpi_port_t rpi_port;

#define RXBUF_SIZE 128
#define RXBUF_MASK (RXBUF_SIZE - 1)

static volatile uint8_t rxBuf[RXBUF_SIZE];
static volatile uint8_t rxHead = 0;
static volatile uint8_t rxTail = 0;
static volatile uint8_t rxOverflow = 0;
static uint32_t last_packet_timestamp = 0;

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

#define START_BYTE 0xAA

typedef enum {
  arm_ID = 0x01,
  disarm_ID = 0x02,
  setDrive_ID = 0x03,
} PacketID_t;

typedef struct __attribute__((packed)) {
  int16_t lhs;
  int16_t rhs;
} setDrive_t;

static uint8_t crc8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0x00;
  for(uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for(uint8_t b = 0; b < 8; b++) {
      crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
    }
  }
  return crc;
}

typedef enum {
  WAIT_START,
  WAIT_ID,
  WAIT_LEN,
  WAIT_PAYLOAD,
  WAIT_CRC
} ParseState_t;

static ParseState_t parseState = WAIT_START;
static uint8_t pktId, pktLen, pktIdx;
static uint8_t pktPayload[32];
static uint8_t crcBuf[2 + 32];

static void packetHandler(uint8_t id, const uint8_t *payload, uint8_t len)
{
  last_packet_timestamp = get_ms_from_boot();
  switch(id) {
    case arm_ID:
      rpi_port.arm = 1;
      break;
    case disarm_ID:
      rpi_port.arm = 0;
      break;
    case setDrive_ID: {
      if(len != sizeof(setDrive_t)) return;
      setDrive_t drive;
      memcpy(&drive, payload, sizeof(drive));
      rpi_port.lhs = drive.lhs / 10.0f;
      rpi_port.rhs = drive.rhs / 10.0f;
    } break;
  }
}

static void parseByte(uint8_t b)
{
  switch(parseState) {
    case WAIT_START:
      if(b == START_BYTE) parseState = WAIT_ID;
      break;

    case WAIT_ID:
      pktId = b;
      crcBuf[0] = b;
      parseState = WAIT_LEN;
      break;

    case WAIT_LEN:
      pktLen = b;
      crcBuf[1] = b;
      pktIdx = 0;
      if(pktLen > sizeof(pktPayload)) {
        parseState = WAIT_START;
      } else if(pktLen == 0) {
        parseState = WAIT_CRC;
      } else {
        parseState = WAIT_PAYLOAD;
      }
      break;

    case WAIT_PAYLOAD:
      pktPayload[pktIdx] = b;
      crcBuf[2 + pktIdx] = b;
      pktIdx++;
      if(pktIdx >= pktLen) parseState = WAIT_CRC;
      break;

    case WAIT_CRC: {
      uint8_t calc = crc8(crcBuf, 2 + pktLen);
      if(calc == b) {
        packetHandler(pktId, pktPayload, pktLen);
      }
      parseState = WAIT_START;
    } break;
  }
}

int rpi_port_init(uint32_t baud)
{
  uart2_init(baud);
  return 0;
}

int rpi_port_update(void)
{
  if(rpi_port.arm == 1 && get_ms_from_boot() - last_packet_timestamp > 1000)
    ABORT(7002);

  uint8_t out;
  while(rxRead(&out)) {
    parseByte(out);
  }
  if(rxOverflow) {
    ABORT(7001);
    rxOverflow = 0;
  }
  return 0;
}
