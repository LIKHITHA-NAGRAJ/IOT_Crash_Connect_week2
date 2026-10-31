#pragma once
#include <stdint.h>

#define CONFIG_PENDING "PENDING_CONFIRMATION"

enum ImuInterface : uint8_t {
  IMU_IF_PENDING = 0,
  IMU_IF_I2C,
  IMU_IF_SPI
};

struct CommsConfig {
  const char *transport;
  const char *apn;
  const char *server_host;
  uint16_t server_port;
  const char *topic_telemetry;
  const char *topic_event;
};

struct TimingConfig {
  uint32_t tick_ms;
  uint32_t boot_ms;
  uint32_t self_test_ms;
  uint32_t imu_sample_ms;
  uint32_t gnss_poll_ms;
  uint32_t telemetry_ms;
  uint32_t event_telemetry_ms;
  uint32_t gps_acquire_timeout_ms;
  uint32_t network_timeout_ms;
  uint32_t gnss_stale_ms;
  uint32_t event_hold_ms;
  uint32_t fault_restart_ms;
};

struct ThresholdConfig {
  float impact_accel_g;
};

struct PinConfig {
  int8_t gnss_rx;
  int8_t gnss_tx;
  int8_t imu_sda;
  int8_t imu_scl;
  int8_t imu_cs;
  int8_t modem_rx;
  int8_t modem_tx;
  int8_t modem_pwrkey;
  int8_t sos_button;
  int8_t battery_adc;
};

struct DeviceConfig {
  const char *device_id;
  const char *fw_version;
  const char *packet_version;
  bool use_simulated_inputs;
  ImuInterface imu_interface;
  CommsConfig comms;
  TimingConfig timing;
  ThresholdConfig threshold;
  PinConfig pins;
};

const DeviceConfig &config();
bool config_is_valid();
