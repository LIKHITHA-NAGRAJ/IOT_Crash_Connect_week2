#include "state_machine.h"
#include <Arduino.h>

namespace DeviceState {
  static State currentState = BOOT;

  // Which transitions are allowed. PENDING CONFIRMATION with the team.
  static bool isAllowed(State from, State to) {
    switch (from) {
      case BOOT:               return to == SELF_TEST;
      case SELF_TEST:          return to == ACQUIRING_GPS || to == FAULT;
      case ACQUIRING_GPS:      return to == NETWORK_CONNECTING || to == DEGRADED || to == FAULT;
      case NETWORK_CONNECTING: return to == OPERATIONAL || to == DEGRADED || to == FAULT;
      case OPERATIONAL:        return to == EVENT_ACTIVE || to == DEGRADED || to == FAULT;
      case EVENT_ACTIVE:       return to == OPERATIONAL || to == DEGRADED || to == FAULT;
      case DEGRADED:           return to == OPERATIONAL || to == FAULT;
      case FAULT:              return to == BOOT;   // reset. PENDING CONFIRMATION
    }
    return false;
  }

  void begin() {
    currentState = BOOT;
    Serial.println("[STATE] Initial state: BOOT");
  }

  State get() {
    return currentState;
  }

  bool transitionTo(State next) {
    if (next < BOOT || next > FAULT) {
      Serial.println("[STATE] Invalid state rejected");
      return false;
    }
    if (next == currentState) {
      return true;
    }
    if (!isAllowed(currentState, next)) {
      Serial.print("[STATE] REJECTED (not allowed): ");
      Serial.print(toString(currentState));
      Serial.print(" -> ");
      Serial.println(toString(next));
      return false;
    }

    Serial.print("[STATE] ");
    Serial.print(toString(currentState));
    Serial.print(" -> ");
    Serial.println(toString(next));

    currentState = next;
    return true;
  }

  // One decision per call, based on the current state and the inputs.
  State step(const Inputs& in) {
    switch (currentState) {
      case BOOT:
        transitionTo(SELF_TEST);
        break;
      case SELF_TEST:
        transitionTo(in.selfTestOk ? ACQUIRING_GPS : FAULT);
        break;
      case ACQUIRING_GPS:
        if (in.gpsFix) transitionTo(NETWORK_CONNECTING);
        break;                       // otherwise keep waiting (no timeout yet)
      case NETWORK_CONNECTING:
        if (in.networkUp) transitionTo(OPERATIONAL);
        break;
      case OPERATIONAL:
        if (!in.gpsFix || !in.networkUp) transitionTo(DEGRADED);
        else if (in.eventDetected)       transitionTo(EVENT_ACTIVE);
        break;
      case EVENT_ACTIVE:
        if (in.eventCleared) transitionTo(OPERATIONAL);
        break;
      case DEGRADED:
        if (in.gpsFix && in.networkUp) transitionTo(OPERATIONAL);
        break;
      case FAULT:
        break;                       // stays until reset (PENDING CONFIRMATION)
    }
    return currentState;
  }

  const char* toString(State state) {
    switch (state) {
      case BOOT: return "BOOT";
      case SELF_TEST: return "SELF_TEST";
      case ACQUIRING_GPS: return "ACQUIRING_GPS";
      case NETWORK_CONNECTING: return "NETWORK_CONNECTING";
      case OPERATIONAL: return "OPERATIONAL";
      case EVENT_ACTIVE: return "EVENT_ACTIVE";
      case DEGRADED: return "DEGRADED";
      case FAULT: return "FAULT";
      default: return "UNKNOWN";
    }
  }
}
