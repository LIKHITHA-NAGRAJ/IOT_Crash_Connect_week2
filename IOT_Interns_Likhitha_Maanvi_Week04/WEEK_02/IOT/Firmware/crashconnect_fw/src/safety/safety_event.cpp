#include "safety_event.h"
#include "../config/config.h"

EventType safety_evaluate(const ImuSample &imu, bool sos_pressed) {
  if (sos_pressed) return EVENT_SOS;
  if (imu.valid && imu.accel_mag_g >= config().threshold.impact_accel_g) {
    return EVENT_IMPACT_SUSPECTED;
  }
  return EVENT_NONE;
}
