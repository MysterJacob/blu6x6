#include <stdio.h>

#include "port.h"
int main()
{
  char cmd[48];
  int code;
  int conv;
  port_init();
  while(1) {
    printf(">>>");
    fflush(stdout);
    while((conv = scanf("%47s", cmd)) == 0) {
    };
    if(conv < 0) return -1;
    code = port_process_command(cmd);
    printf("command exited with code: %d\n", code);
  }
}
