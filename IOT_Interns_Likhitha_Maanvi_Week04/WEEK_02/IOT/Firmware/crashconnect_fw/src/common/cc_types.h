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

enum EventType : uint8_t {
  EVENT_NONE = 0,
  EVENT_IMPACT_SUSPECTED,
  EVENT_SOS
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

inline const char *event_name(EventType e) {
  switch (e) {
    case EVENT_NONE:             return "NONE";
    case EVENT_IMPACT_SUSPECTED: return "IMPACT_SUSPECTED";
    case EVENT_SOS:              return "SOS";
    default:                     return "UNKNOWN";
  }
}
