#pragma once
#include <stdint.h>

enum SosAction : uint8_t {
  SOS_NONE = 0,
  SOS_PRESS_BEGIN,
  SOS_TRIGGERED,
  SOS_RELEASED_EARLY
};

struct SosButton {
  bool stable;
  bool lastRaw;
  uint32_t changedMs;
  uint32_t pressStartMs;
  bool fired;
};

void sos_button_init(SosButton *b);
SosAction sos_button_step(SosButton *b, bool raw_pressed, uint32_t now_ms, uint32_t debounce_ms, uint32_t hold_ms,
                          uint32_t *held_ms);
