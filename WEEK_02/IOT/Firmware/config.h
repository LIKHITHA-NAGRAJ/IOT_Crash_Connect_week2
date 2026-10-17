#pragma once
// config.h - all tunable settings live here, NOT in the logic.
// Values are PLACEHOLDERS unless stated. "PENDING CONFIRMATION" = needs team confirmation.
#include <stdint.h>

struct IntervalConfig {
  uint32_t gnssReadMs;     // how often simulated GNSS data is processed
  uint32_t imuReadMs;      // how often the IMU is read
  uint32_t telemetryMs;    // how often telemetry is sent
  uint32_t statusMs;       // how often device status is printed
};

struct CommsConfig {
  const char* apn;         // PENDING CONFIRMATION
  const char* serverHost;  // PENDING CONFIRMATION
  uint16_t    serverPort;  // PENDING CONFIRMATION
  uint8_t     maxRetries;
  uint32_t    retryDelayMs;
};

struct EventConfig {
  float crashThresholdG;   // PLACEHOLDER, PENDING CONFIRMATION
};

struct TimeoutConfig {
  uint32_t gpsAcquireMs;
  uint32_t networkConnectMs;
  uint32_t gnssStaleMs;
  uint32_t eventHoldMs;
};

struct PinConfig {         // -1 = PENDING CONFIRMATION (pins not decided)
  int gnssRx, gnssTx;
  int imuSda, imuScl;
  int modemRx, modemTx;
};

struct DeviceConfig {
  const char*    deviceId;
  bool           simulateHardware;   // true = recorded/simulated inputs, no hardware
  IntervalConfig intervals;
  CommsConfig    comms;
  EventConfig    event;
  TimeoutConfig  timeouts;
  PinConfig      pins;
};

extern const DeviceConfig CONFIG;