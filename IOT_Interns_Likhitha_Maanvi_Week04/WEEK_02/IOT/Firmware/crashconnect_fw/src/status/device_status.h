#pragma once
#include <stdint.h>
#include "../common/cc_types.h"

struct DeviceStatus {
  DeviceState state;
  uint32_t state_since_ms;
  bool gnss_module_ok;
  bool gnss_fix;
  bool imu_ok;
  bool comms_connected;
  bool sos_active;
  EventType event;
  uint32_t fault_flags;
};

DeviceStatus &status_get();
void status_print_line(uint32_t now_ms);
