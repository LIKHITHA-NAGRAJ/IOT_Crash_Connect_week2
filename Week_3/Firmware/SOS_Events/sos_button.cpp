#include "sos_button.h"
#include <string.h>

void SosButton::begin(const SosConfig& c) {
  memset(this, 0, sizeof *this);
  cfg = c;
}

SosUpdate SosButton::update(bool raw, uint32_t now) {
  SosUpdate u = { false, SosFeedback::NONE, 0 };

  // Debounce: the raw level must stay unchanged for debounceMs before it counts.
  if (raw != lastRaw) { lastRaw = raw; rawChangeMs = now; }
  bool newStable = stable;
  if (now - rawChangeMs >= cfg.debounceMs) newStable = raw;

  if (newStable && !stable) {                       // debounced press edge
    stable = true;
    holding = true;
    pressStartMs = rawChangeMs;                     // hold time counts from the first level change
    consumed = false;
    if (now < cooldownUntil) {
      consumed = true;                              // ignore this press completely
      u.feedback = SosFeedback::IGNORED_COOLDOWN;
      return u;
    }
    u.feedback = SosFeedback::PRESS_STARTED;
    return u;
  }

  if (!newStable && stable) {                       // debounced release edge
    stable = false;
    bool wasHolding = holding && !consumed;
    holding = false;
    if (wasHolding) u.feedback = SosFeedback::RELEASED_EARLY;
    return u;
  }

  if (stable && holding && !consumed) {             // still holding
    uint32_t held = now - pressStartMs;
    if (held >= cfg.longPressMs) {
      consumed = true;
      cooldownUntil = now + cfg.cooldownMs;
      u.triggered = true;
      u.feedback = SosFeedback::TRIGGERED;
      u.progressPct = 100;
    } else {
      u.feedback = SosFeedback::HOLDING;
      u.progressPct = (uint8_t)((held * 100UL) / cfg.longPressMs);
    }
  }
  return u;
}

const char* sosFeedbackName(SosFeedback f) {
  switch (f) {
    case SosFeedback::NONE: return "NONE";
    case SosFeedback::PRESS_STARTED: return "PRESS_STARTED";
    case SosFeedback::HOLDING: return "HOLDING";
    case SosFeedback::TRIGGERED: return "TRIGGERED";
    case SosFeedback::RELEASED_EARLY: return "RELEASED_EARLY";
    case SosFeedback::IGNORED_COOLDOWN: return "IGNORED_COOLDOWN";
  }
  return "?";
}
