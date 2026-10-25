#include <Arduino.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "accident_detector.h"
#include "sim_scenarios.h"
#include "device_integration.h"
#include "state_machine.h"
#include "gnss.h"
#include "tests.h"

// Accident-detection replay harness.
// *** ALL SAMPLES ARE SIMULATED by sim_scenarios.cpp (14 scenarios, 50 Hz, GPS at 1 Hz). ***
// Thresholds are PLACEHOLDERS. Results show how the software logic behaves on made-up data.
// They are NOT evidence of real accident-detection performance and say nothing about real sensors.

static int passed = 0, failed = 0;
static void check(bool ok, const char* label) {
  Serial.print(ok ? "  PASS: " : "  FAIL: ");
  Serial.println(label);
  if (ok) passed++; else failed++;
}

struct Replay {
  AccidentDetector* det;
  int decisions;
  bool anyFalseReason;
};

// Replays one scenario: feeds GPS speed (1 Hz) and IMU samples (50 Hz) in time order.
static void feedOne(const SimSample& s, void* p) {
  Replay* r = (Replay*)p;
  if (s.speedState == 1)      r->det->feedSpeed(true, s.speedKmh);
  else if (s.speedState == 2) r->det->feedSpeed(false, 0.0f);
  ImuInput in = { s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz };
  AccidentResult res;
  if (r->det->feedSample(in, res)) r->decisions++;
}

static const char* verdict(const ScenarioInfo& si, const AccidentStats& st) {
  if (si.ideal) return st.confirmed > 0 ? "DETECTED" : "MISSED";
  if (st.confirmed > 0) return "FALSE_POSITIVE";
  return st.candidates > 0 ? "candidate rejected" : "quiet";
}

static AccidentStats runScenario(int idx, const AccidentConfig& cfg) {
  AccidentDetector det;
  det.begin(cfg);
  Replay r = { &det, 0, false };
  generateScenario(idx, feedOne, &r);
  return det.stats();
}

static void replayTable(const char* title, bool jerk, int& falsePos, int& missed) {
  AccidentConfig cfg = CONFIG.accident;
  cfg.jerkEnabled = jerk;
  Serial.println(title);
  Serial.println("  scenario                   ideal cand conf rej  verdict");
  for (int i = 0; i < scenarioCount(); i++) {
    const ScenarioInfo& si = scenarioInfo(i);
    AccidentStats st = runScenario(i, cfg);
    const char* v = verdict(si, st);
    if (strcmp(v, "FALSE_POSITIVE") == 0) falsePos++;
    if (strcmp(v, "MISSED") == 0) missed++;
    char line[120];
    snprintf(line, sizeof line, "  %-26s %-5s %4d %4d %3d  %s", si.name, si.ideal ? "yes" : "no",
             st.candidates, st.confirmed, st.rejected, v);
    Serial.println(line);
    bool ok = jerk ? (st.candidates == si.expCandidatesJerk)
                   : (st.candidates == si.expCandidates && st.confirmed == si.expConfirmed);
    if (!ok) { Serial.print("  FAIL: behaviour changed for "); Serial.println(si.name); failed++; } else passed++;
  }
}

int runAccidentTests() {
  Serial.println("\n=== ACCIDENT DETECTION REPLAY (SIMULATED samples, placeholder thresholds) ===");

  int fpA = 0, missA = 0, fpB = 0, missB = 0;
  replayTable("\nRun A: default configuration (jerk trigger OFF)", false, fpA, missA);
  replayTable("\nRun B: jerk trigger ON (more candidates expected)", true, fpB, missB);

  Serial.print("\nRun A: false positives = "); Serial.print(fpA);
  Serial.print(", missed = "); Serial.println(missA);
  check(missA == 0, "no ideal accident was missed in run A (simulated)");
  check(fpA == 2, "exactly the 2 documented false-positive risks are reported (bump_then_hard_stop, bump_gps_missing_smooth)");

  // Parameters really are configurable: changing one value changes behaviour.
  {
    AccidentConfig cfg = CONFIG.accident;
    cfg.impactThresholdG = 12.0f;                       // above every simulated impact
    AccidentStats st = runScenario(9, cfg);
    check(st.candidates == 0, "raising the impact threshold removes the frontal-impact candidate");
    cfg = CONFIG.accident;
    cfg.requireOrientation = true;
    AccidentStats a = runScenario(9, cfg);              // frontal: no orientation change, no gyro spike
    AccidentStats b = runScenario(10, cfg);             // rollover-like: both present
    check(a.confirmed == 0 && b.confirmed == 1, "requireOrientation rejects frontal impact but keeps rollover-like");
  }

  // Detector -> firmware: confirmed event reaches the state machine through the integration module.
  Serial.println("\nPipeline to state machine (simulated):");
  DeviceState::begin();
  DeviceState::Inputs ok = { true, true, true, false, false, false };
  for (int i = 0; i < 4; i++) DeviceState::step(ok);
  check(DeviceState::get() == DeviceState::OPERATIONAL, "device is OPERATIONAL before the replay");

  Integration::accidentBegin(CONFIG.accident);
  {
    struct Feed { static void cb(const SimSample& s, void*) {
      if (s.speedState == 1) Integration::feedGpsSpeed(true, s.speedKmh);
      else if (s.speedState == 2) Integration::feedGpsSpeed(false, 0.0f);
      ImuInput in = { s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz };
      Integration::feedImu(in);
    } };
    generateScenario(9, Feed::cb, nullptr);             // frontal impact scenario
  }
  check(Integration::eventLatched(), "confirmed event is latched");
  DeviceState::Inputs in = ok;
  Integration::applyEventLatch(in);
  DeviceState::step(in);
  check(DeviceState::get() == DeviceState::EVENT_ACTIVE, "OPERATIONAL -> EVENT_ACTIVE");
  delay(CONFIG.timeouts.eventHoldMs + 100);
  in = ok;
  Integration::applyEventLatch(in);
  DeviceState::step(in);
  check(DeviceState::get() == DeviceState::OPERATIONAL && !Integration::eventLatched(), "event hold elapsed -> back to OPERATIONAL");

  // An event while DEGRADED (for example GPS lost) must still be handled.
  DeviceState::Inputs lost = ok; lost.gpsFix = false;
  DeviceState::step(lost);
  check(DeviceState::get() == DeviceState::DEGRADED, "GPS lost -> DEGRADED");
  Integration::accidentBegin(CONFIG.accident);
  {
    struct Feed2 { static void cb(const SimSample& s, void*) {
      ImuInput in2 = { s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz };
      if (s.speedState == 1) Integration::feedGpsSpeed(true, s.speedKmh);
      else if (s.speedState == 2) Integration::feedGpsSpeed(false, 0.0f);
      Integration::feedImu(in2);
    } };
    generateScenario(10, Feed2::cb, nullptr);           // rollover-like scenario
  }
  DeviceState::Inputs in2 = lost;
  Integration::applyEventLatch(in2);
  DeviceState::step(in2);
  check(DeviceState::get() == DeviceState::EVENT_ACTIVE, "event while DEGRADED -> EVENT_ACTIVE (not dropped)");

  // A non-event scenario must not reach the state machine.
  DeviceState::begin();
  for (int i = 0; i < 4; i++) DeviceState::step(ok);
  Integration::accidentBegin(CONFIG.accident);
  {
    struct Feed3 { static void cb(const SimSample& s, void*) {
      ImuInput in3 = { s.tMs, s.ax, s.ay, s.az, s.gx, s.gy, s.gz };
      if (s.speedState == 1) Integration::feedGpsSpeed(true, s.speedKmh);
      else if (s.speedState == 2) Integration::feedGpsSpeed(false, 0.0f);
      Integration::feedImu(in3);
    } };
    generateScenario(5, Feed3::cb, nullptr);            // pothole_double_hit
  }
  check(!Integration::eventLatched(), "pothole scenario: no event latched");
  check(Integration::accidentStats().candidates == 1 && Integration::accidentStats().rejected == 1,
        "pothole scenario: one candidate, rejected");

  // GNSS speed (knots) is converted to km/h for the detector.
  GNSS::begin();
  GNSS::feedLine("$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,*47");
  GNSS::feedLine("$GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A");
  Integration::accidentBegin(CONFIG.accident);
  Integration::feedSpeedFromGnss();
  float kmh = 0;
  check(Integration::detectorSpeed(kmh) && kmh > 41.0f && kmh < 42.0f, "recorded GNSS speed 22.4 kn -> 41.5 km/h for the detector");
  GNSS::begin();
  Integration::feedSpeedFromGnss();
  check(!Integration::detectorSpeed(kmh), "no GNSS fix -> speed unavailable for the detector");

  Serial.print("\nRESULT: "); Serial.print(passed); Serial.print(" passed, ");
  Serial.print(failed); Serial.println(" failed");
  Serial.println("NOTE: SIMULATED samples and placeholder thresholds. This is not validated accident detection.");
  return failed;
}
