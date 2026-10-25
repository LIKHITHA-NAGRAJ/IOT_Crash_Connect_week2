#pragma once

#include <stdint.h>
#include "config.h"

// Manual SOS long-press logic: debounce, hold timing, software feedback, cooldown.
// SIMULATED: this module only sees a boolean "pressed" signal and a time. It does not read a pin.
// No physical button, wiring or electrical behaviour is validated. Pin and circuit are PENDING with PCB.

enum class SosFeedback {
  NONE,
  PRESS_STARTED,       // a debounced press began
  HOLDING,             // still holding, see progressPct
  TRIGGERED,           // the long press completed: SOS requested
  RELEASED_EARLY,      // released before the long-press time: no SOS
  IGNORED_COOLDOWN     // pressed during the cooldown after a trigger
};

struct SosUpdate {
  bool        triggered;     // true exactly once per completed long press
  SosFeedback feedback;
  uint8_t     progressPct;   // 0 to 100 while holding
};

class SosButton {
public:
  void begin(const SosConfig& cfg);
  // Call often (for example every 10 ms) with the raw level (true = pressed) and the time.
  SosUpdate update(bool rawPressed, uint32_t nowMs);
  bool pressed() const { return stable; }

private:
  SosConfig cfg;
  bool     lastRaw;
  uint32_t rawChangeMs;
  bool     stable;
  bool     holding;
  bool     consumed;       // the current press already triggered (or was ignored)
  uint32_t pressStartMs;
  uint32_t cooldownUntil;
  bool     inCooldown;
};

const char* sosFeedbackName(SosFeedback f);
