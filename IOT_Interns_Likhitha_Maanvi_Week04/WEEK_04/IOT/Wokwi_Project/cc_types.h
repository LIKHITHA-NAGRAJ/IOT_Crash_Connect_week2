#pragma once
#include <stdint.h>

enum DeviceState : uint8_t {
  STATE_BOOT = 0,
  STATE_SELF_TEST,
  STATE_ACQUIRING_GPS,
  STATE_NETWORK_CONNECTING,
  STATE_OPERATIONAL,
  STATE_EVENT_ACTIVE,
  STATE_DEGRADED,
  STATE_FAULT,
  STATE_COUNT
};

enum EventKind : uint8_t {
  EVENT_NONE = 0,
  EVENT_ACCIDENT,
  EVENT_SOS
};

enum TimeSource : uint8_t {
  TIME_NONE = 0,
  TIME_SIMULATED,
  TIME_GNSS
};

inline const char *state_name(DeviceState s) {
  switch (s) {
    case STATE_BOOT:               return "BOOT";
    case STATE_SELF_TEST:          return "SELF_TEST";
    case STATE_ACQUIRING_GPS:      return "ACQUIRING_GPS";
    case STATE_NETWORK_CONNECTING: return "NETWORK_CONNECTING";
    case STATE_OPERATIONAL:        return "OPERATIONAL";
    case STATE_EVENT_ACTIVE:       return "EVENT_ACTIVE";
    case STATE_DEGRADED:           return "DEGRADED";
    case STATE_FAULT:              return "FAULT";
    default:                       return "UNKNOWN";
  }
}

inline const char *event_kind_name(EventKind e) {
  switch (e) {
    case EVENT_NONE:     return "none";
    case EVENT_ACCIDENT: return "accident";
    case EVENT_SOS:      return "sos";
    default:             return "unknown";
  }
}

inline const char *time_source_name(TimeSource t) {
  switch (t) {
    case TIME_SIMULATED: return "SIMULATED";
    case TIME_GNSS:      return "GNSS";
    default:             return "NONE";
  }
}
