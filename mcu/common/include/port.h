#pragma once
void port_init();

typedef int (*command_handler)(int argc, char **argv);
int port_register_command(const char *name, command_handler callback);
int port_process_command(const char cmd[48]);

typedef struct _port_cmd {
  const char *name;
  command_handler callback;
} port_command;

enum {
  OK = 0,
  COMMAND_BUFFER_OVERFLOW,
  INVALID_ARGUMENT,
  ERR_COMMAND_NOT_FOUND,
  COMMAND_NAME_COLLISION,
  COMMAND_NOT_FOUND
};
