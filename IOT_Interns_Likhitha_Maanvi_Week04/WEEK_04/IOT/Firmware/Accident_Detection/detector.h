#pragma once
#include <stdint.h>
#include "../IMU/imu.h"

enum DetectDecision : uint8_t {
  DET_NONE = 0,
  DET_CANDIDATE,
  DET_CONFIRMED,
  DET_REJECTED
};

struct DetectorConfig {
  float trigger_g;
  uint32_t min_duration_ms;
  float sharp_jerk_g_per_s;
  float sharp_min_g;
  uint32_t window_ms;
  float still_band_g;
  float still_gyro_dps;
  uint32_t still_ms;
  float rotation_deg;
  float prior_moving_kmh;
  float stopped_kmh;
  uint32_t refractory_ms;
};

struct DetectResult {
  DetectDecision decision;
  char accident_type[24];
  char reason[64];
  char speed_check[10];
  float peak_g;
  float max_rotation_deg;
  uint32_t trigger_ms;
  uint32_t decision_after_ms;
};

struct DetectorCounters {
  uint32_t triggers;
  uint32_t confirmed;
  uint32_t rejected;
  uint32_t invalid_samples;
};

DetectorConfig detector_default_config();
void detector_init(const DetectorConfig &cfg);
DetectResult detector_step(const ImuSample &s, bool speed_valid, float speed_kmh);
const DetectorCounters &detector_counters();
