#pragma once

// Device state machine (see Monday state diagram).
// Transitions are checked against an allowed list. Which transitions should exist
// is PENDING CONFIRMATION. Timeouts (e.g. GPS acquire timeout) are NOT implemented yet.

namespace DeviceState {
  enum State {
    BOOT,
    SELF_TEST,
    ACQUIRING_GPS,
    NETWORK_CONNECTING,
    OPERATIONAL,
    EVENT_ACTIVE,
    DEGRADED,
    FAULT
  };

  // Conditions that drive the transitions. For now these are TEST / SIMULATED
  // inputs, not measurements from real hardware.
  struct Inputs {
    bool selfTestOk;
    bool gpsFix;
    bool networkUp;
    bool eventDetected;
    bool eventCleared;
  };

  void begin();
  State get();
  bool transitionTo(State next);        // false if the transition is not allowed
  State step(const Inputs& in);         // apply the rules once using the given inputs
  const char* toString(State state);
}
