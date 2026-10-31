#include <Arduino.h>
#include "device_status.h"

namespace {

DeviceStatus current;

}

DeviceStatus &status_get() {
  return current;
}

void status_print_line(uint32_t now_ms) {
  Serial.printf("[STATUS] t=%.1fs state=%s gnss=%s fix=%s imu=%s comms=%s event=%s sos=%s faults=0x%02lX\n",
                now_ms / 1000.0,
                state_name(current.state),
                current.gnss_module_ok ? "OK" : "NOT_OK",
                current.gnss_fix ? "YES" : "NO",
                current.imu_ok ? "OK" : "NOT_OK",
                current.comms_connected ? "UP" : "DOWN",
                event_name(current.event),
                current.sos_active ? "YES" : "NO",
                (unsigned long)current.fault_flags);
}
