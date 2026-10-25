#include "device_integration.h"
#include "gnss.h"
#include "modem.h"
#include "event_manager.h"
#include <Arduino.h>
#include <stdio.h>
#include "config.h"

namespace Integration {

  // ---------------- accident detector wiring ----------------
  static AccidentDetector detector;
  static AccidentResult   lastResult;
  static bool             haveResult = false;
  static bool             latched = false;
  static uint32_t         latchedMs = 0;

  void accidentBegin(const AccidentConfig& cfg) {
    detector.begin(cfg);
    haveResult = false;
    latched = false;
  }

  void feedGpsSpeed(bool valid, float kmh) { detector.feedSpeed(valid, kmh); }

  void feedSpeedFromGnss() {
    GNSS::Fix f;
    GNSS::getFix(f);
    if (f.valid && f.speedValid) detector.feedSpeed(true, (float)(f.speedKnots * 1.852));   // knots to km/h
    else                         detector.feedSpeed(false, 0.0f);
  }

  bool detectorSpeed(float& kmh) { kmh = detector.speedKmh(); return detector.speedValid(); }

  void feedImu(const ImuInput& s) {
    AccidentResult r;
    if (detector.feedSample(s, r)) {
      lastResult = r;
      haveResult = true;
      if (r.confirmed) {
        Serial.print("[EVENT] ACCIDENT CONFIRMED (simulated/test data): ");
        Serial.println(r.reason);
        Events::createAccident(r, millis());        // queued in any device state, so it is never lost
        if (!latched) { latched = true; latchedMs = millis(); }
      }
    }
  }

  SosUpdate feedSosButton(bool rawPressed, uint32_t nowMs) {
    SosUpdate u = Events::feedButton(rawPressed, nowMs);   // creates the SOS event on a completed press
    if (u.triggered && !latched) { latched = true; latchedMs = millis(); }
    return u;
  }

  bool eventLatched() { return latched; }
  AccidentStats accidentStats() { return detector.stats(); }
  const AccidentResult* lastDecision() { return haveResult ? &lastResult : nullptr; }

  void applyEventLatch(DeviceState::Inputs& in) {
    if (!latched) return;
    uint32_t el = millis() - latchedMs;
    DeviceState::State st = DeviceState::get();
    if (st == DeviceState::EVENT_ACTIVE) {
      if (el >= CONFIG.timeouts.eventHoldMs) { in.eventCleared = true; latched = false; }
    } else if (st == DeviceState::OPERATIONAL || st == DeviceState::DEGRADED) {
      in.eventDetected = true;
    } else if (el >= 2 * CONFIG.timeouts.eventHoldMs) {
      latched = false;      // device was still starting up. State policy is PENDING CONFIRMATION.
      Serial.println("[EVENT] state change skipped: device was not OPERATIONAL or DEGRADED. The event itself stays queued.");
    }
  }

  // ---------------- state-machine inputs ----------------

  DeviceState::Inputs makeInputs(bool selfTestOk, bool eventDetected, bool eventCleared) {
    GNSS::Fix fix;
    GNSS::getFix(fix);                       // valid only for a CURRENT, usable fix
    Modem::Status ms = Modem::getStatus();

    DeviceState::Inputs in = {};
    in.selfTestOk    = selfTestOk;
    in.gpsFix        = fix.valid;
    in.networkUp     = (ms.state == Modem::READY_SIMULATED);   // MOCK ready, not a real connection
    in.networkFailed = Modem::isFailed();
    in.eventDetected = eventDetected;
    in.eventCleared  = eventCleared;
    applyEventLatch(in);                     // adds the detector's confirmed event, if any
    return in;
  }

  void statusText(char* out, size_t size) {
    GNSS::Fix fix;
    GNSS::getFix(fix);
    Modem::Status ms = Modem::getStatus();
    const char* net = (ms.state == Modem::READY_SIMULATED) ? "MOCK_READY"
                    : Modem::isFailed() ? "MOCK_FAILED" : "MOCK_CONNECTING";
    snprintf(out, size, "GPS=%s net=%s(%s) state=%s",
             GNSS::statusText(fix), net, Modem::stateName(ms.state),
             DeviceState::toString(DeviceState::get()));
  }
}
