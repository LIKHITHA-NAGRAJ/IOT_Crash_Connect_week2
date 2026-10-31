#include "device_state.h"
#include "../config/config.h"

namespace {

DeviceState currentState = STATE_BOOT;
uint32_t enteredMs = 0;

bool all_good(const StateInputs &in) {
  return in.gps_fix && in.network_up && !in.noncritical_fault;
}

}

DeviceState state_next(DeviceState current, const StateInputs &in, const char **reason) {
  const TimingConfig &t = config().timing;
  const char *dummy = 0;
  if (reason == 0) reason = &dummy;
  *reason = 0;

  if (current != STATE_FAULT && in.critical_fault) {
    *reason = "critical fault";
    return STATE_FAULT;
  }

  switch (current) {
    case STATE_BOOT:
      if (in.time_in_state_ms >= t.boot_ms) {
        *reason = "boot complete";
        return STATE_SELF_TEST;
      }
      break;

    case STATE_SELF_TEST:
      if (in.time_in_state_ms >= t.self_test_ms) {
        if (in.self_test_pass) {
          *reason = "self-test passed";
          return STATE_ACQUIRING_GPS;
        }
        *reason = "self-test failed";
        return STATE_FAULT;
      }
      break;

    case STATE_ACQUIRING_GPS:
      if (in.gps_fix) {
        *reason = "GPS fix acquired";
        return STATE_NETWORK_CONNECTING;
      }
      if (in.time_in_state_ms >= t.gps_acquire_timeout_ms) {
        *reason = "GPS acquire timeout";
        return STATE_DEGRADED;
      }
      break;

    case STATE_NETWORK_CONNECTING:
      if (in.network_up) {
        if (in.gps_fix) {
          *reason = "network connected";
          return STATE_OPERATIONAL;
        }
        *reason = "network connected, GPS fix missing";
        return STATE_DEGRADED;
      }
      if (in.time_in_state_ms >= t.network_timeout_ms) {
        *reason = "network connect timeout";
        return STATE_DEGRADED;
      }
      break;

    case STATE_OPERATIONAL:
      if (in.event != EVENT_NONE) {
        *reason = "event detected";
        return STATE_EVENT_ACTIVE;
      }
      if (!in.gps_fix) {
        *reason = "GPS fix lost";
        return STATE_DEGRADED;
      }
      if (!in.network_up) {
        *reason = "network lost";
        return STATE_DEGRADED;
      }
      if (in.noncritical_fault) {
        *reason = "non-critical fault";
        return STATE_DEGRADED;
      }
      break;

    case STATE_EVENT_ACTIVE:
      if (in.event == EVENT_NONE && in.time_in_state_ms >= t.event_hold_ms) {
        if (all_good(in)) {
          *reason = "event cleared";
          return STATE_OPERATIONAL;
        }
        *reason = "event cleared, system degraded";
        return STATE_DEGRADED;
      }
      break;

    case STATE_DEGRADED:
      if (in.event != EVENT_NONE) {
        *reason = "event detected while degraded";
        return STATE_EVENT_ACTIVE;
      }
      if (all_good(in)) {
        *reason = "recovered";
        return STATE_OPERATIONAL;
      }
      break;

    case STATE_FAULT:
      if (!in.critical_fault && in.time_in_state_ms >= t.fault_restart_ms) {
        *reason = "fault cleared, restarting";
        return STATE_BOOT;
      }
      break;

    default:
      break;
  }
  return current;
}

void state_init(uint32_t now_ms) {
  currentState = STATE_BOOT;
  enteredMs = now_ms;
}

DeviceState state_current() {
  return currentState;
}

uint32_t state_entered_ms() {
  return enteredMs;
}

bool state_update(StateInputs in, uint32_t now_ms, DeviceState *previous, const char **reason) {
  in.time_in_state_ms = now_ms - enteredMs;
  DeviceState next = state_next(currentState, in, reason);
  if (next == currentState) return false;
  if (previous) *previous = currentState;
  currentState = next;
  enteredMs = now_ms;
  return true;
}
