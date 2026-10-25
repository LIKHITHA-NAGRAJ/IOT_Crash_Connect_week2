#pragma once

#include <stdint.h>
#include "config.h"

// Staged accident-detection pipeline (FIRST IMPLEMENTATION, software only).
//
//   Stage 1  candidate trigger: impact magnitude held for a minimum duration (or jerk)
//   Stage 2  orientation change: gravity vector before versus after, and a gyroscope spike
//   Stage 3  post-event stillness and GPS speed drop (when speed is available)
//   Stage 4  confirmation window: the decision is made when the window ends
//
// All thresholds come from AccidentConfig and are PLACEHOLDERS, not validated settings.
// This file has no Arduino dependency, so it can be replayed on a PC with CSV data.
// Results from simulated data say nothing about real sensor behaviour or real crashes.

struct ImuInput {
  uint32_t tMs;                 // sample time in milliseconds
  float ax, ay, az;             // acceleration in g
  float gx, gy, gz;             // angular rate in degrees per second
};

struct AccidentResult {
  bool     confirmed;           // false = the candidate was rejected
  const char* reason;
  uint32_t candidateMs;
  uint32_t decisionMs;
  bool     triggeredByJerk;
  float    peakG;               // highest magnitude seen after the trigger
  float    peakGyroDps;         // highest angular rate in the settle period
  bool     gyroSpike;
  bool     orientationChanged;  // only evaluated when the device was quiet at the end
  float    orientationDeg;
  bool     stillness;
  bool     preSpeedKnown;       // GPS speed was available at the trigger
  bool     postSpeedKnown;
  float    preSpeedKmh;
  float    postSpeedKmh;
  bool     wasMoving;           // only meaningful when preSpeedKnown
  bool     speedDrop;
  bool     lowConfidence;       // confirmed without any GPS speed information
};

struct AccidentStats {
  uint16_t candidates;
  uint16_t confirmed;
  uint16_t rejected;
};

class AccidentDetector {
public:
  void begin(const AccidentConfig& cfg);
  // Call whenever GPS speed is updated. valid=false means "speed unavailable" (no fix or stale).
  void feedSpeed(bool valid, float kmh);
  // Feed samples in time order. Returns true when a FINAL decision (confirmed or rejected) is made.
  bool feedSample(const ImuInput& s, AccidentResult& out);

  AccidentStats stats() const { return st; }
  bool speedValid() const { return spdValid; }
  float speedKmh() const { return spdKmh; }
  bool confirming() const { return inWindow; }

private:
  AccidentConfig cfg;
  AccidentStats st;

  // speed
  bool  spdValid;
  float spdKmh;

  // sample history and gravity estimate
  bool     hasPrev;
  float    prevMag;
  uint32_t prevT;
  bool     gInit;
  float    gEst[3];

  // stage 1
  bool     above;
  uint32_t aboveStart;
  float    runPeak;
  uint32_t cooldownUntil;

  // candidate window (stages 2 to 4)
  bool     inWindow;
  uint32_t candT;
  bool     byJerk;
  float    peakG, peakGyro;
  float    gravBefore[3];
  bool     gravBeforeValid;
  bool     preSpdKnown;
  float    preSpd;
  bool     quiet;
  uint32_t quietSince;
  float    sumA[3];
  uint32_t nSum;

  void startCandidate(const ImuInput& s, float mag, bool jerk);
  void decide(uint32_t now, AccidentResult& out);
};
