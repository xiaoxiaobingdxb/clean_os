#ifndef TOOL_LOG_H
#define TOOL_LOG_H

#include "../config.h"

int init_log();

typedef enum {
    LogLevelDebug,
    LogLevelInfo,
    LogLevelWarn,
    LogLevelErr,
    LogLevelFatal
}log_level;

void set_level(log_level level);
void log_printf(log_level level, const char* fmt, ...);
void log_debug(const char* fmt, ...);
void log_info(const char* fmt, ...);
void log_warn(const char* fmt, ...);
void log_err(const char* fmt, ...);
void log_fatal(const char* fmt, ...);

#if LOG_SHOW_PATH
#define _log_debug(fmt, args...) \
log_debug("DEBUG(%s:%d:%s):", __FILE__, __LINE__, __func__);\
log_debug(fmt, ##args);
#else
#define _log_debug(fmt, args...) \
log_debug(fmt, ##args);
#endif

#endif

