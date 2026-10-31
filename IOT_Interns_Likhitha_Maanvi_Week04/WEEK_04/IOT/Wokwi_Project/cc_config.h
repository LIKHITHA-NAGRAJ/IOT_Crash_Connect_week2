#pragma once
#include <stdint.h>

struct DeviceConfig {
  char device_id[32];
  bool provisioned;
  uint32_t rpt_moving_s;
  uint32_t rpt_stationary_s;
  uint32_t rpt_event_s;
  uint32_t move_dkmh;
  uint32_t q_tel_cap;
  uint32_t q_evt_cap;
  uint32_t backoff_init_s;
  uint32_t backoff_max_s;
  uint32_t ack_timeout_ms;
  uint32_t sos_hold_ms;
  uint32_t sos_debounce_ms;
  uint32_t wd_timeout_s;
  uint32_t log_level;
  uint32_t boot_count;
  char apn[32];
  char server_url[96];
};

struct Timing {
  uint32_t tick_ms;
  uint32_t boot_ms;
  uint32_t self_test_ms;
  uint32_t imu_sample_ms;
  uint32_t gnss_stale_ms;
  uint32_t gps_acquire_timeout_ms;
  uint32_t network_timeout_ms;
  uint32_t event_hold_ms;
  uint32_t fault_restart_ms;
  uint32_t modem_boot_wait_ms;
  uint32_t modem_cmd_timeout_ms;
  uint32_t modem_register_timeout_ms;
  uint32_t modem_http_timeout_ms;
  uint32_t modem_reg_check_ms;
};

struct ConfigReport {
  int loaded;
  int defaulted;
};

const char *const CFG_PENDING = "PENDING_CONFIRMATION";
const int MAX_QUEUE_CAP = 50;

ConfigReport config_load();
DeviceConfig &config();
const Timing &timing();
bool config_provision(const char *device_id);
bool config_set_u32(const char *key, uint32_t value);
uint32_t config_bump_boot_count();
void config_describe(char *out, int cap);
