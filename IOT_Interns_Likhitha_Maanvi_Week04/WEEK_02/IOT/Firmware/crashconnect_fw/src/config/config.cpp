#include "config.h"
#include <string.h>

namespace {

DeviceConfig build_defaults() {
  DeviceConfig c;
  memset(&c, 0, sizeof(c));

  c.device_id = "CRASH_ESP32_01";
  c.fw_version = "0.2.0-week2";
  c.packet_version = "0.1-DRAFT";
  c.use_simulated_inputs = true;
  c.imu_interface = IMU_IF_PENDING;

  c.comms.transport = CONFIG_PENDING;
  c.comms.apn = CONFIG_PENDING;
  c.comms.server_host = CONFIG_PENDING;
  c.comms.server_port = 0;
  c.comms.topic_telemetry = CONFIG_PENDING;
  c.comms.topic_event = CONFIG_PENDING;

  c.timing.tick_ms = 100;
  c.timing.boot_ms = 1000;
  c.timing.self_test_ms = 1000;
  c.timing.imu_sample_ms = 500;
  c.timing.gnss_poll_ms = 1000;
  c.timing.telemetry_ms = 5000;
  c.timing.event_telemetry_ms = 1000;
  c.timing.gps_acquire_timeout_ms = 30000;
  c.timing.network_timeout_ms = 30000;
  c.timing.gnss_stale_ms = 3000;
  c.timing.event_hold_ms = 4000;
  c.timing.fault_restart_ms = 3000;

  c.threshold.impact_accel_g = 4.0f;

  c.pins.gnss_rx = -1;
  c.pins.gnss_tx = -1;
  c.pins.imu_sda = -1;
  c.pins.imu_scl = -1;
  c.pins.imu_cs = -1;
  c.pins.modem_rx = -1;
  c.pins.modem_tx = -1;
  c.pins.modem_pwrkey = -1;
  c.pins.sos_button = -1;
  c.pins.battery_adc = -1;

  return c;
}

}

const DeviceConfig &config() {
  static const DeviceConfig cfg = build_defaults();
  return cfg;
}

bool config_is_valid() {
  const DeviceConfig &c = config();
  if (c.device_id == 0 || c.device_id[0] == '\0') return false;
  if (c.timing.tick_ms == 0 || c.timing.imu_sample_ms == 0) return false;
  if (c.timing.gnss_poll_ms == 0 || c.timing.telemetry_ms == 0) return false;
  if (c.timing.event_telemetry_ms == 0 || c.timing.gnss_stale_ms == 0) return false;
  if (c.threshold.impact_accel_g <= 0.0f) return false;
  return true;
}
