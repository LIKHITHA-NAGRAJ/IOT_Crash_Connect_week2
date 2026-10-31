#pragma once
#include <stddef.h>
#include <stdint.h>
#include "../common/cc_types.h"
#include "../gnss/gnss.h"
#include "../imu/imu.h"

struct TelemetryInput {
  const GnssFix *gnss;
  const ImuSample *imu;
  DeviceState state;
  EventType event;
  bool sos_active;
  bool comms_connected;
  bool battery_valid;
  uint16_t battery_mv;
  uint32_t fault_flags;
  uint32_t uptime_ms;
  uint32_t seq;
};

uint32_t telemetry_next_seq();
size_t telemetry_build_json(const TelemetryInput &in, char *buf, size_t cap);
