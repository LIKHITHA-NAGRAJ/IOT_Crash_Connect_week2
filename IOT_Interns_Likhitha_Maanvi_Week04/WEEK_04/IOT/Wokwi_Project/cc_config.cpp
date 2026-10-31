#include <stdio.h>
#include <string.h>
#include "cc_config.h"
#include "cc_log.h"
#include "cc_kv_store.h"

namespace {

DeviceConfig cfg;

const Timing kTiming = {20, 1000, 1000, 20, 3000, 30000, 60000, 10000, 5000, 11000, 1000, 30000, 8000, 3000};

struct U32Key {
  const char *key;
  uint32_t DeviceConfig::*field;
  uint32_t def;
  uint32_t lo;
  uint32_t hi;
};

const U32Key kKeys[] = {
  {"rpt_move_s",   &DeviceConfig::rpt_moving_s,     5,    1,   3600},
  {"rpt_still_s",  &DeviceConfig::rpt_stationary_s, 30,   1,   3600},
  {"rpt_event_s",  &DeviceConfig::rpt_event_s,      2,    1,   600},
  {"move_dkmh",    &DeviceConfig::move_dkmh,        50,   5,   500},
  {"q_tel_cap",    &DeviceConfig::q_tel_cap,        50,   1,   (uint32_t)MAX_QUEUE_CAP},
  {"q_evt_cap",    &DeviceConfig::q_evt_cap,        10,   1,   (uint32_t)MAX_QUEUE_CAP},
  {"bo_init_s",    &DeviceConfig::backoff_init_s,   1,    1,   60},
  {"bo_max_s",     &DeviceConfig::backoff_max_s,    30,   1,   600},
  {"ack_to_ms",    &DeviceConfig::ack_timeout_ms,   5000, 500, 60000},
  {"sos_hold_ms",  &DeviceConfig::sos_hold_ms,      2000, 500, 10000},
  {"sos_deb_ms",   &DeviceConfig::sos_debounce_ms,  50,   10,  500},
  {"wd_timeout_s", &DeviceConfig::wd_timeout_s,     8,    2,   120},
  {"log_level",    &DeviceConfig::log_level,        3,    0,   4},
  {"boot_count",   &DeviceConfig::boot_count,       0,    0,   0xFFFFFFFFu}
};

const int kKeyCount = (int)(sizeof(kKeys) / sizeof(kKeys[0]));

void set_defaults() {
  memset(&cfg, 0, sizeof(cfg));
  snprintf(cfg.device_id, sizeof(cfg.device_id), "UNPROVISIONED");
  cfg.provisioned = false;
  snprintf(cfg.apn, sizeof(cfg.apn), "%s", CFG_PENDING);
  snprintf(cfg.server_url, sizeof(cfg.server_url), "%s", CFG_PENDING);
  for (int i = 0; i < kKeyCount; ++i) cfg.*(kKeys[i].field) = kKeys[i].def;
}

}

ConfigReport config_load() {
  ConfigReport r = {0, 0};
  set_defaults();
  if (!kv_begin("crashconn")) {
    LOGE("CONFIG", ERR_STORAGE_FAILED, "storage not available, defaults used");
    r.defaulted = kKeyCount + 3;
    return r;
  }
  char buf[96];
  if (kv_get_str("device_id", buf, sizeof(buf)) && buf[0] != '\0' && strlen(buf) < sizeof(cfg.device_id)) {
    snprintf(cfg.device_id, sizeof(cfg.device_id), "%.31s", buf);
    cfg.provisioned = true;
    r.loaded++;
  } else {
    LOGW("CONFIG", ERR_CONFIG_DEFAULTED, "device_id missing, device is UNPROVISIONED");
    r.defaulted++;
  }
  if (kv_get_str("apn", buf, sizeof(buf)) && strlen(buf) < sizeof(cfg.apn)) {
    snprintf(cfg.apn, sizeof(cfg.apn), "%.31s", buf);
    r.loaded++;
  } else {
    r.defaulted++;
  }
  if (kv_get_str("server_url", buf, sizeof(buf)) && strlen(buf) < sizeof(cfg.server_url)) {
    snprintf(cfg.server_url, sizeof(cfg.server_url), "%.95s", buf);
    r.loaded++;
  } else {
    r.defaulted++;
  }
  for (int i = 0; i < kKeyCount; ++i) {
    uint32_t v = 0;
    if (kv_get_u32(kKeys[i].key, &v)) {
      if (v >= kKeys[i].lo && v <= kKeys[i].hi) {
        cfg.*(kKeys[i].field) = v;
        r.loaded++;
      } else {
        LOGW("CONFIG", ERR_CONFIG_DEFAULTED, "%s=%lu out of range, default %lu used", kKeys[i].key,
             (unsigned long)v, (unsigned long)kKeys[i].def);
        r.defaulted++;
      }
    } else {
      r.defaulted++;
    }
  }
  if (cfg.rpt_moving_s > cfg.rpt_stationary_s) {
    LOGW("CONFIG", ERR_CONFIG_DEFAULTED, "moving interval larger than stationary, both reset to defaults");
    cfg.rpt_moving_s = 5;
    cfg.rpt_stationary_s = 30;
    r.defaulted += 2;
  }
  log_set_level((LogLevel)cfg.log_level);
  return r;
}

DeviceConfig &config() {
  return cfg;
}

const Timing &timing() {
  return kTiming;
}

bool config_provision(const char *device_id) {
  if (device_id == 0 || device_id[0] == '\0' || strlen(device_id) >= sizeof(cfg.device_id)) return false;
  if (!kv_put_str("device_id", device_id)) {
    LOGE("CONFIG", ERR_STORAGE_FAILED, "could not store device_id");
    return false;
  }
  snprintf(cfg.device_id, sizeof(cfg.device_id), "%s", device_id);
  cfg.provisioned = true;
  return true;
}

bool config_set_u32(const char *key, uint32_t value) {
  for (int i = 0; i < kKeyCount; ++i) {
    if (strcmp(kKeys[i].key, key) != 0) continue;
    if (value < kKeys[i].lo || value > kKeys[i].hi) return false;
    DeviceConfig trial = cfg;
    trial.*(kKeys[i].field) = value;
    if (trial.rpt_moving_s > trial.rpt_stationary_s) return false;
    if (!kv_put_u32(key, value)) return false;
    cfg = trial;
    if (strcmp(key, "log_level") == 0) log_set_level((LogLevel)value);
    return true;
  }
  return false;
}

uint32_t config_bump_boot_count() {
  uint32_t n = cfg.boot_count + 1;
  kv_put_u32("boot_count", n);
  cfg.boot_count = n;
  return n;
}

void config_describe(char *out, int cap) {
  snprintf(out, (size_t)cap,
           "id=%s provisioned=%d report moving=%lus stationary=%lus event=%lus move_threshold=%.1fkm/h "
           "queues tel=%lu evt=%lu backoff=%lus..%lus ack_timeout=%lums sos_hold=%lums debounce=%lums wd=%lus boot=%lu",
           cfg.device_id, cfg.provisioned ? 1 : 0, (unsigned long)cfg.rpt_moving_s, (unsigned long)cfg.rpt_stationary_s,
           (unsigned long)cfg.rpt_event_s, cfg.move_dkmh / 10.0, (unsigned long)cfg.q_tel_cap,
           (unsigned long)cfg.q_evt_cap, (unsigned long)cfg.backoff_init_s, (unsigned long)cfg.backoff_max_s,
           (unsigned long)cfg.ack_timeout_ms, (unsigned long)cfg.sos_hold_ms, (unsigned long)cfg.sos_debounce_ms,
           (unsigned long)cfg.wd_timeout_s, (unsigned long)cfg.boot_count);
}
