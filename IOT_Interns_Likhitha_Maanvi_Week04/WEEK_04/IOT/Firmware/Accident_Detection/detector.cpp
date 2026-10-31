#include <math.h>
#include <string.h>
#include <stdio.h>
#include "detector.h"

namespace {

enum Stage { IDLE, OBSERVE, REFRACTORY };

DetectorConfig cfg;
DetectorCounters counters;
Stage stage = IDLE;

bool gravityPrimed = false;
float gx0 = 0, gy0 = 0, gz0 = 1;
float prevMag = 1.0f;
uint32_t prevT = 0;
bool havePrev = false;
uint32_t runHiStart = 0;
bool runHi = false;
uint32_t runLoStart = 0;
bool runLo = false;
bool runLoSharp = false;
bool preSpeedValid = false;
float preSpeed = 0;

uint32_t tTrigger = 0;
uint32_t tRefractoryEnd = 0;
float peakG = 0;
float maxRot = 0;
bool stillRun = false;
uint32_t stillStart = 0;
bool stillReached = false;
float trigGx = 0, trigGy = 0, trigGz = 1;
bool preSpeedAtTriggerValid = false;
float preSpeedAtTrigger = 0;

DetectResult make(DetectDecision d, const char *type, const char *reason, uint32_t now) {
  DetectResult r;
  memset(&r, 0, sizeof(r));
  r.decision = d;
  snprintf(r.accident_type, sizeof(r.accident_type), "%s", type);
  snprintf(r.reason, sizeof(r.reason), "%s", reason);
  snprintf(r.speed_check, sizeof(r.speed_check), "UNKNOWN");
  r.peak_g = peakG;
  r.max_rotation_deg = maxRot;
  r.trigger_ms = tTrigger;
  r.decision_after_ms = now - tTrigger;
  return r;
}

float angle_deg(float ax, float ay, float az, float bx, float by, float bz) {
  float na = sqrtf(ax * ax + ay * ay + az * az);
  float nb = sqrtf(bx * bx + by * by + bz * bz);
  if (na < 0.01f || nb < 0.01f) return 0.0f;
  float c = (ax * bx + ay * by + az * bz) / (na * nb);
  if (c > 1.0f) c = 1.0f;
  if (c < -1.0f) c = -1.0f;
  return acosf(c) * 57.29578f;
}

}

DetectorConfig detector_default_config() {
  DetectorConfig c;
  c.trigger_g = 3.0f;
  c.min_duration_ms = 20;
  c.sharp_jerk_g_per_s = 100.0f;
  c.sharp_min_g = 2.0f;
  c.window_ms = 5000;
  c.still_band_g = 0.15f;
  c.still_gyro_dps = 20.0f;
  c.still_ms = 1000;
  c.rotation_deg = 60.0f;
  c.prior_moving_kmh = 15.0f;
  c.stopped_kmh = 5.0f;
  c.refractory_ms = 10000;
  return c;
}

void detector_init(const DetectorConfig &c) {
  cfg = c;
  memset(&counters, 0, sizeof(counters));
  stage = IDLE;
  gravityPrimed = false;
  havePrev = false;
  runHi = runLo = runLoSharp = false;
  preSpeedValid = false;
}

const DetectorCounters &detector_counters() {
  return counters;
}

DetectResult detector_step(const ImuSample &s, bool speedValid, float speedKmh) {
  DetectResult none = make(DET_NONE, "", "", s.t_ms);
  if (!s.valid || isnan(s.mag_g)) {
    counters.invalid_samples++;
    return none;
  }
  uint32_t t = s.t_ms;

  if (stage == REFRACTORY) {
    if (t >= tRefractoryEnd) stage = IDLE;
    return none;
  }

  if (stage == IDLE) {
    if (!gravityPrimed) {
      gx0 = s.ax; gy0 = s.ay; gz0 = s.az;
      gravityPrimed = true;
    } else if (s.mag_g < 1.3f) {
      gx0 += 0.02f * (s.ax - gx0);
      gy0 += 0.02f * (s.ay - gy0);
      gz0 += 0.02f * (s.az - gz0);
    }
    if (speedValid) {
      preSpeedValid = true;
      preSpeed = speedKmh;
    }

    bool sharp = false;
    if (havePrev && t > prevT && (t - prevT) <= 100) {
      float jerk = (s.mag_g - prevMag) / ((float)(t - prevT) / 1000.0f);
      sharp = jerk >= cfg.sharp_jerk_g_per_s;
    }
    havePrev = true;
    prevMag = s.mag_g;
    prevT = t;

    if (s.mag_g >= cfg.trigger_g) {
      if (!runHi) { runHi = true; runHiStart = t; }
    } else {
      runHi = false;
    }
    if (s.mag_g >= cfg.sharp_min_g) {
      if (!runLo) { runLo = true; runLoStart = t; runLoSharp = sharp; }
    } else {
      runLo = false;
      runLoSharp = false;
    }

    bool trigHi = runHi && (t - runHiStart) >= cfg.min_duration_ms;
    bool trigLo = runLo && runLoSharp && (t - runLoStart) >= cfg.min_duration_ms;
    if (!trigHi && !trigLo) return none;

    stage = OBSERVE;
    counters.triggers++;
    tTrigger = t;
    peakG = s.mag_g;
    maxRot = 0;
    stillRun = false;
    stillReached = false;
    trigGx = gx0; trigGy = gy0; trigGz = gz0;
    preSpeedAtTriggerValid = preSpeedValid;
    preSpeedAtTrigger = preSpeed;
    runHi = runLo = runLoSharp = false;
    return make(DET_CANDIDATE, "", trigHi ? "sustained high g" : "sharp onset, sustained g", t);
  }

  if (s.mag_g > peakG) peakG = s.mag_g;
  bool quiet = fabsf(s.mag_g - 1.0f) <= cfg.still_band_g &&
               fabsf(s.gx) <= cfg.still_gyro_dps && fabsf(s.gy) <= cfg.still_gyro_dps && fabsf(s.gz) <= cfg.still_gyro_dps;
  if (quiet) {
    float rot = angle_deg(trigGx, trigGy, trigGz, s.ax, s.ay, s.az);
    if (rot > maxRot) maxRot = rot;
    if (!stillRun) { stillRun = true; stillStart = t; }
    if (!stillReached && (t - stillStart) >= cfg.still_ms) stillReached = true;
  } else {
    stillRun = false;
  }

  bool windowOver = (t - tTrigger) >= cfg.window_ms;
  const char *type = maxRot >= cfg.rotation_deg ? "ROLLOVER_SUSPECTED" : "IMPACT";

  if (stillReached) {
    bool speedHigh = speedValid && speedKmh > cfg.stopped_kmh;
    if (!speedHigh || windowOver) {
      if (speedHigh) {
        counters.rejected++;
        stage = REFRACTORY;
        tRefractoryEnd = t + cfg.refractory_ms;
        DetectResult r = make(DET_REJECTED, "", "still after impact but GPS speed still high", t);
        snprintf(r.speed_check, sizeof(r.speed_check), "NO");
        return r;
      }
      counters.confirmed++;
      stage = REFRACTORY;
      tRefractoryEnd = t + cfg.refractory_ms;
      DetectResult r = make(DET_CONFIRMED, type, "impact followed by stillness", t);
      if (speedValid && preSpeedAtTriggerValid) {
        bool ok = preSpeedAtTrigger >= cfg.prior_moving_kmh && speedKmh <= cfg.stopped_kmh;
        snprintf(r.speed_check, sizeof(r.speed_check), ok ? "YES" : "NO");
      }
      return r;
    }
  }
  if (windowOver) {
    counters.rejected++;
    stage = REFRACTORY;
    tRefractoryEnd = t + cfg.refractory_ms;
    DetectResult r = make(DET_REJECTED, "", "no stillness after impact within window", t);
    if (speedValid && speedKmh > cfg.stopped_kmh) snprintf(r.speed_check, sizeof(r.speed_check), "NO");
    return r;
  }
  return none;
}
