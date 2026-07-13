#define BAUD 9600
#define LINE_BUF_SIZE 64

void port_setup_serial();
char port_getchar(void);
char *uart_readline(void);
