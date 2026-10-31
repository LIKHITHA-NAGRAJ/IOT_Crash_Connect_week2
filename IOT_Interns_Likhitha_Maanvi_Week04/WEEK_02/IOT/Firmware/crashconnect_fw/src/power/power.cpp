#include <Arduino.h>
#include "power.h"

namespace {

bool warned = false;

}

bool power_read_battery_mv(uint16_t *mv) {
  (void)mv;
  if (!warned) {
    Serial.println("[POWER][STUB] battery voltage not available (ADC pin and divider: PENDING CONFIRMATION)");
    warned = true;
  }
  return false;
}
