#include <stdint.h>
#define BUFFER_SIZE 64

typedef struct {
  uint8_t arm;
  uint8_t dir;
  float rhs;
  float lhs;
} rpi_port_t;

extern rpi_port_t rpi_port;

int rpi_port_init(int baudrate);
int rpi_port_update();
