#pragma once

#include "state_machine.h"
#include "accident_detector.h"
#include "sos_button.h"
#include <stddef.h>

// The ONE place where GNSS and modem results are turned into state-machine inputs.
// Keeps modem.* and gnss.* independent of each other and of the state machine.
//
// NETWORK NOTE: "networkUp" here means the MOCK modem reached READY_SIMULATED.
// It is NOT a real LTE registration or data connection.

namespace Integration {
  // Reads the current GNSS fix and modem status and builds the inputs for DeviceState::step().
  DeviceState::Inputs makeInputs(bool selfTestOk, bool eventDetected, bool eventCleared);

  // ---- Accident detector wiring (SIMULATED or recorded inputs only) ----
  void accidentBegin(const AccidentConfig& cfg);
  void feedImu(const ImuInput& s);                 // runs the staged detector; latches a confirmed event
  void feedGpsSpeed(bool valid, float kmh);        // speed in km/h; valid=false = unavailable
  void feedSpeedFromGnss();                        // GNSS speed (knots) converted to km/h; no valid fix = unavailable
  bool detectorSpeed(float& kmh);                  // current speed seen by the detector (false if unavailable)
  // Manual SOS: feeds the long-press logic; a completed press creates an SOS event and latches the state change.
  SosUpdate feedSosButton(bool rawPressed, uint32_t nowMs);
  bool eventLatched();
  AccidentStats accidentStats();
  const AccidentResult* lastDecision();            // nullptr until a decision has been made
  // Adds the event inputs (eventDetected / eventCleared) from the latched detector decision.
  void applyEventLatch(DeviceState::Inputs& in);

  // One-line summary, e.g. "GPS=VALID net=MOCK_READY(READY_SIMULATED) state=OPERATIONAL"
  void statusText(char* out, size_t size);
}
