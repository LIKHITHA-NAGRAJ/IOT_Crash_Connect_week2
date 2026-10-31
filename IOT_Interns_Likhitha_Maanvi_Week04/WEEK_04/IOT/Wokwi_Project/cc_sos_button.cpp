#include "cc_sos_button.h"

void sos_button_init(SosButton *b) {
  b->stable = false;
  b->lastRaw = false;
  b->changedMs = 0;
  b->pressStartMs = 0;
  b->fired = false;
}

SosAction sos_button_step(SosButton *b, bool raw, uint32_t now, uint32_t debounce, uint32_t hold, uint32_t *held) {
  if (held) *held = 0;
  if (raw != b->lastRaw) {
    b->lastRaw = raw;
    b->changedMs = now;
  }
  if (raw != b->stable && (now - b->changedMs) >= debounce) {
    b->stable = raw;
    if (raw) {
      b->pressStartMs = b->changedMs;
      b->fired = false;
      return SOS_PRESS_BEGIN;
    }
    uint32_t heldMs = b->changedMs - b->pressStartMs;
    if (held) *held = heldMs;
    bool wasFired = b->fired;
    b->fired = false;
    return wasFired ? SOS_NONE : SOS_RELEASED_EARLY;
  }
  if (b->stable && !b->fired && (now - b->pressStartMs) >= hold) {
    b->fired = true;
    if (held) *held = now - b->pressStartMs;
    return SOS_TRIGGERED;
  }
  return SOS_NONE;
}
