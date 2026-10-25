#include "imu.h"
#include <Arduino.h>

namespace IMU {
  static Reading testReading = {
    0.02f, 0.01f, 0.98f,
    0.50f, 0.20f, 1.00f
  };

  bool begin() {
    Serial.println("[IMU] SIMULATED module initialized");
    return true;
  }

  Reading readData() {
    // Test values only; not readings from a physical sensor.
    Serial.println("[IMU] SIMULATED TEST DATA");
    return testReading;
  }

  float accelerationMagnitude(const Reading& reading) {
    return sqrtf(
      reading.ax * reading.ax +
      reading.ay * reading.ay +
      reading.az * reading.az
    );
  }
}