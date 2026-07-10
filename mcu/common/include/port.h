#pragma once
void port_init();

typedef int (*command_handler_t)(int argc, char **argv);
int port_register_command(const char *name, command_handler_t callback);
int port_process_command(const char cmd[48]);

typedef struct _port_cmd {
  const char *name;
  command_handler_t callback;
} port_command_T;

enum {
  OK = 0,
  COMMAND_BUFFER_OVERFLOW,
  INVALID_ARGUMENT,
  ERR_COMMAND_NOT_FOUND,
  COMMAND_NAME_COLLISION,
  COMMAND_NOT_FOUND
};
