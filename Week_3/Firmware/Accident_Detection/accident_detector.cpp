#include "accident_detector.h"
#include <math.h>
#include <string.h>

static float len3(const float v[3]) { return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]); }

void AccidentDetector::begin(const AccidentConfig& c) {
  memset(this, 0, sizeof *this);
  cfg = c;
}

void AccidentDetector::feedSpeed(bool valid, float kmh) {
  spdValid = valid;
  spdKmh = valid ? kmh : 0.0f;
}

void AccidentDetector::startCandidate(const ImuInput& s, float mag, bool jerk) {
  inWindow = true;
  candT = s.tMs;
  byJerk = jerk;
  peakG = (runPeak > mag) ? runPeak : mag;
  peakGyro = 0;
  quiet = false;
  quietSince = 0;
  sumA[0] = sumA[1] = sumA[2] = 0;
  nSum = 0;
  gravBeforeValid = gInit;
  memcpy(gravBefore, gEst, sizeof gravBefore);
  preSpdKnown = spdValid;
  preSpd = spdKmh;
  st.candidates++;
  above = false;
}

bool AccidentDetector::feedSample(const ImuInput& s, AccidentResult& out) {
  const float a[3] = { s.ax, s.ay, s.az };
  const float mag = len3(a);
  const float gyro[3] = { s.gx, s.gy, s.gz };
  const float gmag = len3(gyro);
  const float dtMs = hasPrev ? (float)(s.tMs - prevT) : 0.0f;
  bool decided = false;

  if (!inWindow) {
    // Slow gravity-vector estimate, updated only while the signal is close to 1 g.
    if (!gInit) {
      memcpy(gEst, a, sizeof gEst);
      gInit = true;
    } else if (fabsf(mag - 1.0f) < cfg.quietBandG && dtMs > 0) {
      float alpha = dtMs / ((float)cfg.gravityTauMs + dtMs);
      for (int i = 0; i < 3; i++) gEst[i] += alpha * (a[i] - gEst[i]);
    }

    // Stage 1: candidate trigger
    if (s.tMs >= cooldownUntil) {
      bool trig = false, jerkTrig = false;
      if (mag >= cfg.impactThresholdG) {
        if (!above) { above = true; aboveStart = s.tMs; runPeak = 0; }
        if (mag > runPeak) runPeak = mag;
        if (s.tMs - aboveStart >= cfg.impactMinMs) trig = true;
      } else {
        above = false;
      }
      if (!trig && cfg.jerkEnabled && hasPrev && dtMs > 0) {
        float jerk = fabsf(mag - prevMag) / (dtMs / 1000.0f);
        if (jerk >= cfg.jerkThresholdGps) { trig = true; jerkTrig = true; }
      }
      if (trig) startCandidate(s, mag, jerkTrig);
    } else {
      above = false;
    }
  } else {
    // Stages 2 to 4: collect evidence until the confirmation window ends
    const uint32_t el = s.tMs - candT;
    if (mag > peakG) peakG = mag;
    if (el <= cfg.settleMs && gmag > peakGyro) peakGyro = gmag;

    if (el < cfg.settleMs) {
      quiet = false;                               // ignore ringing right after the impact
    } else {
      bool q = fabsf(mag - 1.0f) <= cfg.stillBandG && gmag <= cfg.stillGyroDps;
      if (q) { if (!quiet) { quiet = true; quietSince = s.tMs; } }
      else   { quiet = false; }
    }

    if (el + cfg.orientationSampleMs >= cfg.confirmWindowMs) {   // final window: average gravity
      sumA[0] += a[0]; sumA[1] += a[1]; sumA[2] += a[2];
      nSum++;
    }

    if (el >= cfg.confirmWindowMs) {
      decide(s.tMs, out);
      decided = true;
    }
  }

  prevMag = mag;
  prevT = s.tMs;
  hasPrev = true;
  return decided;
}

void AccidentDetector::decide(uint32_t now, AccidentResult& out) {
  memset(&out, 0, sizeof out);
  out.candidateMs = candT;
  out.decisionMs = now;
  out.triggeredByJerk = byJerk;
  out.peakG = peakG;
  out.peakGyroDps = peakGyro;
  out.gyroSpike = peakGyro >= cfg.gyroSpikeDps;

  // Stage 3: stillness (IMU quiet for long enough) and speed information
  const bool quietOk = quiet && (now - quietSince >= cfg.stillnessMinMs);
  out.preSpeedKnown = preSpdKnown;
  out.postSpeedKnown = spdValid;
  out.preSpeedKmh = preSpd;
  out.postSpeedKmh = spdKmh;
  const bool stillMoving = spdValid && spdKmh > cfg.postSpeedMaxKmh;
  out.stillness = quietOk && !stillMoving;
  out.wasMoving = preSpdKnown && preSpd >= cfg.minPreSpeedKmh;
  out.speedDrop = preSpdKnown && spdValid && preSpd >= cfg.minPreSpeedKmh &&
                  (preSpd - spdKmh) >= cfg.speedDropDeltaKmh && spdKmh <= cfg.postSpeedMaxKmh;

  // Stage 2: orientation change (only meaningful when the device is quiet at the end)
  if (quietOk && gravBeforeValid && nSum > 0) {
    float mean[3] = { sumA[0] / nSum, sumA[1] / nSum, sumA[2] / nSum };
    float lm = len3(mean), lb = len3(gravBefore);
    if (lm > 0.5f && lb > 0.5f) {
      float c = (mean[0] * gravBefore[0] + mean[1] * gravBefore[1] + mean[2] * gravBefore[2]) / (lm * lb);
      if (c > 1.0f) c = 1.0f;
      if (c < -1.0f) c = -1.0f;
      out.orientationDeg = acosf(c) * 57.2957795f;
      out.orientationChanged = out.orientationDeg >= cfg.orientationChangeDeg;
    }
  }

  // Stage 4: classification
  bool confirmed = false;
  if (preSpdKnown && !out.wasMoving) {
    out.reason = "rejected: vehicle not moving before impact";
  } else if (!(out.stillness || out.speedDrop)) {
    out.reason = stillMoving ? "rejected: vehicle still moving after impact"
                             : "rejected: no stillness or speed drop";
  } else if (cfg.requireOrientation && !(out.orientationChanged || out.gyroSpike)) {
    out.reason = "rejected: no orientation change or gyro spike (required)";
  } else {
    confirmed = true;
    out.lowConfidence = !(preSpdKnown && spdValid);
    out.reason = out.lowConfidence ? "confirmed: stillness only (no GPS speed, low confidence)"
               : (out.stillness && out.speedDrop) ? "confirmed: stillness and speed drop"
               : out.speedDrop ? "confirmed: speed drop" : "confirmed: stillness";
  }
  out.confirmed = confirmed;

  if (confirmed) st.confirmed++; else st.rejected++;
  inWindow = false;
  cooldownUntil = now + cfg.cooldownMs;
  above = false;
}
