#include "port.h"

#include <stdio.h>
#include <string.h>

#include "fail.h"

#define PORT_MAX_COMMANDS 32
#define PORT_CMD_BUF_LEN 48
#define PORT_MAX_ARGS 8

static port_command_T port_commands[PORT_MAX_COMMANDS];
static int port_command_count = 0;

static int help_command_handler(int argc, char **argv)
{
  for(int i = 0; i < port_command_count; i++) {
    printf("%d. %s\n", i, port_commands[i].name);
  }
  return OK;
}

void port_init(void)
{
  port_command_count = 0;
  memset(port_commands, 0, sizeof(port_commands));
  ON_ERROR_ABORT(port_register_command("help", help_command_handler));
}

int port_register_command(const char *name, command_handler_t callback)
{
  if(!name || !callback) return -1;
  if(port_command_count >= PORT_MAX_COMMANDS) return COMMAND_BUFFER_OVERFLOW;

  for(int i = 0; i < port_command_count; i++) {
    if(strcmp(port_commands[i].name, name) == 0) return COMMAND_NAME_COLLISION;
  }

  port_commands[port_command_count].name = name;
  port_commands[port_command_count].callback = callback;
  port_command_count++;
  return OK;
}

int port_process_command(const char cmd[PORT_CMD_BUF_LEN])
{
  if(!cmd || cmd[0] == '\0') return -1;

  char buf[PORT_CMD_BUF_LEN];
  strncpy(buf, cmd, PORT_CMD_BUF_LEN - 1);
  buf[PORT_CMD_BUF_LEN - 1] = '\0';

  char *argv[PORT_MAX_ARGS];
  int argc = 0;

  char *tok = strtok(buf, " \t");
  while(tok && argc < PORT_MAX_ARGS) {
    argv[argc++] = tok;
    tok = strtok(NULL, " \t");
  }

  for(int i = 0; i < port_command_count; i++) {
    if(strcmp(port_commands[i].name, argv[0]) == 0)
      return port_commands[i].callback(argc, argv);
  }

  return COMMAND_NOT_FOUND;
}
