#include <Arduino.h>
#include "sos.h"
#include "../config/config.h"

namespace {

bool initialised = false;
bool simPressed = false;
bool hwWarned = false;

}

bool sos_init() {
  simPressed = false;
  initialised = true;
  return true;
}

bool sos_is_pressed() {
  if (config().use_simulated_inputs) return simPressed;
  if (!hwWarned) {
    Serial.println("[SOS][STUB] button read not implemented (pin, debounce, long-press rule: PENDING CONFIRMATION)");
    hwWarned = true;
  }
  return false;
}

void sos_sim_set(bool pressed) {
  simPressed = pressed;
}

bool sos_module_ok() {
  return initialised;
}
