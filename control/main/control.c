#include <math.h>
#include <string.h>

#include "ESP_CRSF.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define CRSF_UART_NUM UART_NUM_0
#define CRSF_TX_PIN 21
#define CRSF_RX_PIN 20
#define CRSF_BAUDRATE 420000
#define BUF_SIZE 256

#define MEGA_UART_NUM UART_NUM_1
#define MEGA_TX_PIN 5
#define MEGA_RX_PIN 6
#define MEGA_BAUDRATE 9600

#define CRSF_RAW_MIN 172
#define CRSF_RAW_MAX 1811

#define OUT_MIN -1000
#define OUT_MAX 1000

#define ARM_THRESHOLD 750

static bool armed = false;

#define START_BYTE 0xAA

typedef enum {
  arm_ID = 0x01,
  disarm_ID = 0x02,
  setDrive_ID = 0x03,
} PacketID_t;

typedef struct __attribute__((packed)) {
  int16_t lhs;  // left wheel speed *10
  int16_t rhs;  // right wheel speed *10
} setDrive_t;

static void mega_uart_init(void)
{
  uart_config_t uart_config = {
      .baud_rate = MEGA_BAUDRATE,
      .data_bits = UART_DATA_8_BITS,

      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  uart_driver_install(MEGA_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 0, NULL, 0);
  uart_param_config(MEGA_UART_NUM, &uart_config);
  uart_set_pin(MEGA_UART_NUM, MEGA_TX_PIN, MEGA_RX_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);
}

static float map_range(int32_t value, int32_t in_min, int32_t in_max,
                       float out_min, float out_max)
{
  if(value < in_min) value = in_min;
  if(value > in_max) value = in_max;

  return (float)(value - in_min) * (out_max - out_min) /
             (float)(in_max - in_min) +
         out_min;
}

static float map_channel(uint16_t raw)
{
  return map_range(raw, CRSF_RAW_MIN, CRSF_RAW_MAX, OUT_MIN, OUT_MAX);
}
void mega_send_arm();
void mega_send_disarm();
void mega_send_setDrive(float lhsPercent, float rhsPercent);

void set_speed(const float rhs, const float lhs)
{
  // OUT_MIN/OUT_MAX are -1000..1000, so divide by 10 to get -100..100 %
  float rhsPercent = rhs / 10.0f;
  float lhsPercent = lhs / 10.0f;

  // Optional: clamp to [-100, 100]
  if(rhsPercent > 100.0f) rhsPercent = 100.0f;
  if(rhsPercent < -100.0f) rhsPercent = -100.0f;
  if(lhsPercent > 100.0f) lhsPercent = 100.0f;
  if(lhsPercent < -100.0f) lhsPercent = -100.0f;

  ESP_LOGI("CTRL", "right_speed=%.1f%%", rhsPercent);
  ESP_LOGI("CTRL", "left_speed=%.1f%%", lhsPercent);

  mega_send_setDrive(lhsPercent, rhsPercent);
}
void on_arm(void)
{
  mega_send_arm();
}

void on_disarm(void)
{
  mega_send_disarm();
  mega_send_setDrive(0.0f, 0.0f);
}
static uint8_t crc8(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0x00;
  for(uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for(uint8_t b = 0; b < 8; b++) {
      if(crc & 0x80) {
        crc = (uint8_t)((crc << 1) ^ 0x07);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

// ---------- Generic packet sender ----------

static void mega_send_packet(uint8_t id, const void *payload, uint8_t len)
{
  uint8_t buf[2 + 32];  // ID + LEN + payload
  if(len > 32) return;  // safety, must match Mega’s parser

  buf[0] = id;
  buf[1] = len;
  if(len) {
    memcpy(&buf[2], payload, len);
  }

  uint8_t crc = crc8(buf, (uint8_t)(2 + len));

  // Frame: [START_BYTE][ID][LEN][PAYLOAD...][CRC]
  uint8_t start = START_BYTE;
  uart_write_bytes(MEGA_UART_NUM, (const char *)&start, 1);
  uart_write_bytes(MEGA_UART_NUM, (const char *)buf, (size_t)(2 + len));
  uart_write_bytes(MEGA_UART_NUM, (const char *)&crc, 1);
}

// ---------- High-level helpers (call these from your code) ----------

// Call once during init (e.g. in app_main) before using other functions
void mega_link_init(void)
{
  mega_uart_init();
}

// Arm robot
void mega_send_arm(void)
{
  mega_send_packet(arm_ID, NULL, 0);
}

void mega_send_disarm(void)
{
  mega_send_packet(disarm_ID, NULL, 0);
}

void mega_send_setDrive(float lhsPercent, float rhsPercent)
{
  if(lhsPercent > 100.0f) lhsPercent = 100.0f;
  if(lhsPercent < -100.0f) lhsPercent = -100.0f;
  if(rhsPercent > 100.0f) rhsPercent = 100.0f;
  if(rhsPercent < -100.0f) rhsPercent = -100.0f;

  setDrive_t pkt;
  pkt.lhs = (int16_t)lrintf(lhsPercent * 10.0f);
  pkt.rhs = (int16_t)lrintf(rhsPercent * 10.0f);

  mega_send_packet(setDrive_ID, &pkt, sizeof(pkt));
}
static void update_arming(const crsf_channels_t *channels)
{
  bool switch_high = channels->ch5 > ARM_THRESHOLD;

  if(switch_high && !armed) {
    armed = true;
    on_arm();
  } else if(!switch_high && armed) {
    armed = false;
    on_disarm();
  }
}

static void steering(const crsf_channels_t *channels)
{
  float throttle = map_channel(channels->ch2);
  float roll = map_channel(channels->ch1);

  if(!armed) {
    set_speed(0.0f, 0.0f);
    return;
  }

  float left = throttle + roll;
  float right = throttle - roll;

  if(left > OUT_MAX) left = OUT_MAX;
  if(left < OUT_MIN) left = OUT_MIN;
  if(right > OUT_MAX) right = OUT_MAX;
  if(right < OUT_MIN) right = OUT_MIN;

  set_speed(right, left);
}

void control_loop(const crsf_channels_t *channels)
{
  update_arming(channels);
  steering(channels);
}

void crsf_rx_task(void *arg)
{
  crsf_config_t config = {
      .uart_num = CRSF_UART_NUM, .tx_pin = CRSF_TX_PIN, .rx_pin = CRSF_RX_PIN};
  CRSF_init(&config);

  crsf_channels_t channels = {0};
  while(1) {
    CRSF_receive_channels(&channels);
    control_loop(&channels);
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

void app_main(void)
{
  mega_link_init();
  xTaskCreate(crsf_rx_task, "crsf_rx_task", 4096, NULL, 10, NULL);
}
