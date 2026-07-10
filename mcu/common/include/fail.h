void __attribute__((noreturn)) __abort(const char *line, int code);
void __log_error(const char *line, int code);

#define STR(x) #x
#define ON_ERROR_ABORT(c)                        \
  if((c) != 0) {                                 \
    __abort(__FILE_NAME__ " " STR(__LINE__), c); \
  }

#define ON_ERROR_LOG(c)                              \
  if((c) != 0) {                                     \
    __log_error(__FILE_NAME__ " " STR(__LINE__), c); \
  }
