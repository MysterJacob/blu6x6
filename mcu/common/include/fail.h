void __attribute__((noreturn)) __abort(const char *line, int code);
void __log_error(const char *line, int code);

#define STR(x) #x
#define ABORT(c) __abort(__FILE__ " " STR(__LINE__), c)
#define ON_ERROR_ABORT(c) \
  if((c) != 0) {          \
    ABORT(c);             \
  }

#define ON_ERROR_LOG(c) \
  if((c) != 0) {        \
    ABORT(c);           \
  }
