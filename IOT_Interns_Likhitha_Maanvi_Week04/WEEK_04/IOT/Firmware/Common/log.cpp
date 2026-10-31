#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include "log.h"

namespace {

LogLevel gLevel = CCLOG_INFO;
uint32_t gCounts[5] = {0, 0, 0, 0, 0};
ErrCode gLastError = ERR_NONE;

char level_char(LogLevel l) {
  switch (l) {
    case CCLOG_ERROR: return 'E';
    case CCLOG_WARN:  return 'W';
    case CCLOG_INFO:  return 'I';
    case CCLOG_DEBUG: return 'D';
    default:        return '-';
  }
}

}

void log_set_level(LogLevel level) {
  gLevel = level;
}

LogLevel log_level() {
  return gLevel;
}

void log_message(LogLevel level, const char *tag, ErrCode code, const char *fmt, ...) {
  if (level == CCLOG_NONE || level > CCLOG_DEBUG) return;
  gCounts[level]++;
  if (level == CCLOG_ERROR || level == CCLOG_WARN) gLastError = code;
  if (level > gLevel) return;

  char msg[800];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(msg, sizeof(msg), fmt, ap);
  va_end(ap);

  unsigned long ms = millis();
  if (level == CCLOG_ERROR || level == CCLOG_WARN) {
    Serial.printf("%6lu.%03lu [%c][%s] E%u %s: %s\n", ms / 1000UL, ms % 1000UL, level_char(level), tag,
                  (unsigned)code, err_name(code), msg);
  } else {
    Serial.printf("%6lu.%03lu [%c][%s] %s\n", ms / 1000UL, ms % 1000UL, level_char(level), tag, msg);
  }
}

uint32_t log_count(LogLevel level) {
  return level <= CCLOG_DEBUG ? gCounts[level] : 0;
}

ErrCode log_last_error() {
  return gLastError;
}

void log_reset_counters() {
  for (int i = 0; i < 5; ++i) gCounts[i] = 0;
  gLastError = ERR_NONE;
}
