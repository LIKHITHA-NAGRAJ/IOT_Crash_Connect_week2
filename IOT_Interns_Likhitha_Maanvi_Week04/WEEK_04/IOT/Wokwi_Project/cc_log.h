#pragma once
#include <stdint.h>
#include "cc_errors.h"

enum LogLevel : uint8_t {
  CCLOG_NONE = 0,
  CCLOG_ERROR,
  CCLOG_WARN,
  CCLOG_INFO,
  CCLOG_DEBUG
};

void log_set_level(LogLevel level);
LogLevel log_level();
void log_message(LogLevel level, const char *tag, ErrCode code, const char *fmt, ...) __attribute__((format(printf, 4, 5)));
uint32_t log_count(LogLevel level);
ErrCode log_last_error();
void log_reset_counters();

#define LOGE(tag, code, ...) log_message(CCLOG_ERROR, tag, code, __VA_ARGS__)
#define LOGW(tag, code, ...) log_message(CCLOG_WARN, tag, code, __VA_ARGS__)
#define LOGI(tag, ...) log_message(CCLOG_INFO, tag, ERR_NONE, __VA_ARGS__)
#define LOGD(tag, ...) log_message(CCLOG_DEBUG, tag, ERR_NONE, __VA_ARGS__)
