#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "imu.h"
#include "../config/config.h"

namespace {

struct RawImu {
  float ax, ay, az, gx, gy, gz;
};

const int SAMPLES_PER_CASE = 5;

const RawImu kTestData[IMU_CASE_COUNT][SAMPLES_PER_CASE] = {
  {{0.02f, 0.01f, 0.98f, 0.5f, 0.2f, 1.0f},
   {0.01f, 0.02f, 0.99f, 0.4f, 0.3f, 0.9f},
   {0.02f, 0.00f, 1.00f, 0.6f, 0.1f, 1.1f},
   {0.01f, 0.01f, 0.98f, 0.5f, 0.2f, 1.0f},
   {0.02f, 0.02f, 0.99f, 0.4f, 0.2f, 0.8f}},
  {{0.35f, 0.15f, 0.92f, 12.0f, 5.0f, 3.0f},
   {0.45f, 0.20f, 0.88f, 18.0f, 7.0f, 4.0f},
   {0.55f, 0.25f, 0.80f, 25.0f, 9.0f, 5.0f},
   {0.45f, 0.20f, 0.88f, 18.0f, 7.0f, 4.0f},
   {0.35f, 0.15f, 0.92f, 12.0f, 5.0f, 3.0f}},
  {{1.20f, 0.40f, 0.80f, 45.0f, 20.0f, 10.0f},
   {1.80f, -0.60f, 1.10f, 90.0f, 55.0f, 30.0f},
   {-2.10f, 1.40f, 0.40f, 150.0f, 95.0f, 60.0f},
   {2.40f, -1.60f, 1.90f, 210.0f, 140.0f, 85.0f},
   {0.90f, 0.30f, 1.00f, 60.0f, 30.0f, 15.0f}},
  {{3.80f, -2.90f, 2.60f, 280.0f, 190.0f, 120.0f},
   {4.50f, -3.10f, 3.20f, 320.0f, 210.0f, 150.0f},
   {2.20f, -1.40f, 1.90f, 160.0f, 100.0f, 70.0f},
   {1.10f, -0.60f, 1.20f, 80.0f, 50.0f, 30.0f},
   {0.40f, 0.20f, 1.00f, 30.0f, 15.0f, 10.0f}}
};

ImuCase currentCase = IMU_CASE_STATIONARY;
int sampleIndex = 0;
bool initialised = false;
bool hwWarned = false;

}

bool imu_init() {
  currentCase = IMU_CASE_STATIONARY;
  sampleIndex = 0;
  initialised = true;
  return true;
}

float imu_accel_magnitude(float ax, float ay, float az) {
  return sqrtf(ax * ax + ay * ay + az * az);
}

void imu_sim_set_case(ImuCase c) {
  if (c < 0 || c >= IMU_CASE_COUNT) return;
  currentCase = c;
  sampleIndex = 0;
}

const char *imu_case_name(ImuCase c) {
  switch (c) {
    case IMU_CASE_STATIONARY: return "STATIONARY";
    case IMU_CASE_TILT:       return "TILT_ROTATION";
    case IMU_CASE_MOVEMENT:   return "MOVEMENT";
    case IMU_CASE_IMPACT:     return "IMPACT_TEST";
    default:                  return "UNKNOWN";
  }
}

bool imu_hw_read_lsm6dsox(ImuSample *out) {
  (void)out;
  if (!hwWarned) {
    Serial.println("[IMU][STUB] LSM6DSOX hardware read not implemented (I2C/SPI choice and pins: PENDING CONFIRMATION)");
    hwWarned = true;
  }
  return false;
}

bool imu_read(ImuSample *out) {
  memset(out, 0, sizeof(*out));

  if (config().use_simulated_inputs) {
    const RawImu &r = kTestData[currentCase][sampleIndex % SAMPLES_PER_CASE];
    sampleIndex++;
    out->ax = r.ax; out->ay = r.ay; out->az = r.az;
    out->gx = r.gx; out->gy = r.gy; out->gz = r.gz;
    out->accel_mag_g = imu_accel_magnitude(r.ax, r.ay, r.az);
    out->valid = true;
    out->simulated = true;
    return true;
  }

  if (!imu_hw_read_lsm6dsox(out)) {
    out->valid = false;
    return false;
  }
  out->accel_mag_g = imu_accel_magnitude(out->ax, out->ay, out->az);
  out->valid = true;
  return true;
}

bool imu_module_ok() {
  return initialised;
}
