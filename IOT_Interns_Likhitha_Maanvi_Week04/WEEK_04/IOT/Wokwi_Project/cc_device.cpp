#include <Arduino.h>
#include <string.h>
#include "cc_device.h"

#ifndef CC_USE_HW_WATCHDOG
#define CC_USE_HW_WATCHDOG 0
#endif

#if defined(ARDUINO_ARCH_ESP32)
#include <esp_system.h>
#include <esp_task_wdt.h>
#endif

namespace {

bool haveTime = false;
uint32_t baseEpoch = 0;
uint32_t baseMs = 0;
TimeSource baseSrc = TIME_NONE;
char simReason[16] = "POWERON";
bool reasonOverride = false;
uint32_t wdTimeoutMs = 0;
uint32_t wdLastFeed = 0;

const uint32_t kMinPlausible = 1767225600u;
const uint32_t kMaxPlausible = 4102444800u;

}

uint32_t epoch_from_ymdhms(int y, int mo, int d, int h, int mi, int s) {
  y -= mo <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153u * (unsigned)(mo + (mo > 2 ? -3 : 9)) + 2u) / 5u + (unsigned)d - 1u;
  unsigned doe = yoe * 365u + yoe / 4u - yoe / 100u + doy;
  long days = (long)era * 146097L + (long)doe - 719468L;
  return (uint32_t)(days * 86400L + h * 3600L + mi * 60L + s);
}

void clock_reset() {
  haveTime = false;
  baseSrc = TIME_NONE;
}

void clock_set(uint32_t epoch, TimeSource src, uint32_t now_ms) {
  if (src == TIME_GNSS && (epoch < kMinPlausible || epoch > kMaxPlausible)) return;
  haveTime = true;
  baseEpoch = epoch;
  baseMs = now_ms;
  baseSrc = src;
}

bool clock_now(uint32_t now_ms, uint32_t *epoch, TimeSource *src) {
  if (!haveTime) {
    if (src) *src = TIME_NONE;
    return false;
  }
  if (epoch) *epoch = baseEpoch + (now_ms - baseMs) / 1000u;
  if (src) *src = baseSrc;
  return true;
}

const char *reset_reason_name() {
  if (reasonOverride) return simReason;
#if defined(ARDUINO_ARCH_ESP32)
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT";
    case ESP_RST_SW:        return "SW";
    case ESP_RST_PANIC:     return "PANIC";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT";
    default:                return "UNKNOWN";
  }
#else
  return simReason;
#endif
}

void reset_reason_sim_set(const char *name) {
  snprintf(simReason, sizeof(simReason), "%s", name);
  reasonOverride = true;
}

bool wd_init(uint32_t timeout_s, uint32_t now_ms) {
  wdTimeoutMs = timeout_s * 1000u;
  wdLastFeed = now_ms;
#if defined(ARDUINO_ARCH_ESP32) && CC_USE_HW_WATCHDOG
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_task_wdt_config_t c = {timeout_s * 1000u, 0, true};
  esp_task_wdt_reconfigure(&c);
#else
  esp_task_wdt_init(timeout_s, true);
#endif
  esp_task_wdt_add(NULL);
#endif
  return true;
}

void wd_feed(uint32_t now_ms) {
  wdLastFeed = now_ms;
#if defined(ARDUINO_ARCH_ESP32) && CC_USE_HW_WATCHDOG
  esp_task_wdt_reset();
#endif
}

bool wd_sim_expired(uint32_t now_ms) {
  return wdTimeoutMs > 0 && (now_ms - wdLastFeed) > wdTimeoutMs;
}
